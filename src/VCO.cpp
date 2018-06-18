
#include "VCO.hpp"
#include "dsp/pitch.hpp"

void VCO::onReset() {
	_syncTrigger.reset();
	_modulationStep = modulationSteps;
}

void VCO::onSampleRateChange() {
	setSampleRate(engineGetSampleRate());
	_modulationStep = modulationSteps;
}

void VCO::step() {
	lights[SLOW_LIGHT].value = _slowMode = params[SLOW_PARAM].value > 0.5f;
	_fmLinearMode = params[FM_TYPE_PARAM].value < 0.5f;

	if (!(
		outputs[SQUARE_OUTPUT].active ||
		outputs[SAW_OUTPUT].active ||
		outputs[TRIANGLE_OUTPUT].active ||
		outputs[SINE_OUTPUT].active
	)) {
		return;
	}

	++_modulationStep;
	if (_modulationStep >= modulationSteps) {
		_modulationStep = 0;

		_baseVOct = params[FREQUENCY_PARAM].value;
		_baseVOct += params[FINE_PARAM].value / 12.0f;
		if (inputs[PITCH_INPUT].active) {
			_baseVOct += clamp(inputs[PITCH_INPUT].value, -5.0f, 5.0f);
		}
		if (_slowMode) {
			_baseVOct -= 7.0f;
		}
		_baseHz = cvToFrequency(_baseVOct);

		float pw = params[PW_PARAM].value;
		if (inputs[PW_INPUT].active) {
			pw *= clamp(inputs[PW_INPUT].value / 5.0f, -1.0f, 1.0f);
		}
		pw *= 1.0f - 2.0f * _square.minPulseWidth;
		pw *= 0.5f;
		pw += 0.5f;
		_square.setPulseWidth(_squarePulseWidthSL.next(pw));

		_fmDepth = params[FM_PARAM].value;
	}

	if (_syncTrigger.next(inputs[SYNC_INPUT].value)) {
		_phasor.resetPhase();
	}

	float frequency = _baseHz;
	Phasor::phase_delta_t phaseOffset = 0;
	if (inputs[FM_INPUT].active && _fmDepth > 0.01f) {
		float fm = inputs[FM_INPUT].value * _fmDepth;
		if (_fmLinearMode) {
			phaseOffset = Phasor::radiansToPhase(2.0f * fm);
		}
		else {
			frequency = cvToFrequency(_baseVOct + fm);
		}
	}
	setFrequency(frequency);

	const float oversampleWidth = 100.0f;
	float mix, oMix;
	if (frequency > _oversampleThreshold) {
		if (frequency > _oversampleThreshold + oversampleWidth) {
			mix = 0.0f;
			oMix = 1.0f;
		}
		else {
			oMix = (frequency - _oversampleThreshold) / oversampleWidth;
			mix = 1.0f - oMix;
		}
	}
	else {
		mix = 1.0f;
		oMix = 0.0f;
	}

	float squareOut = 0.0f;
	float sawOut = 0.0f;
	float triangleOut = 0.0f;
	if (oMix > 0.0f) {
		for (int i = 0; i < oversample; ++i) {
			_phasor.advancePhase();
			if (outputs[SQUARE_OUTPUT].active) {
				_squareBuffer[i] = _square.nextFromPhasor(_phasor, phaseOffset);
			}
			if (outputs[SAW_OUTPUT].active) {
				_sawBuffer[i] = _saw.nextFromPhasor(_phasor, phaseOffset);
			}
			if (outputs[TRIANGLE_OUTPUT].active) {
				_triangleBuffer[i] = _triangle.nextFromPhasor(_phasor, phaseOffset);
			}
		}
		if (outputs[SQUARE_OUTPUT].active) {
			squareOut += oMix * amplitude * _squareDecimator.next(_squareBuffer);
		}
		if (outputs[SAW_OUTPUT].active) {
			sawOut += oMix * amplitude * _sawDecimator.next(_sawBuffer);
		}
		if (outputs[TRIANGLE_OUTPUT].active) {
			triangleOut += oMix * amplitude * _triangleDecimator.next(_triangleBuffer);
		}
	}
	else {
		_phasor.advancePhase(oversample);
	}
	if (mix > 0.0f) {
		if (outputs[SQUARE_OUTPUT].active) {
			squareOut += mix * amplitude * _square.nextFromPhasor(_phasor, phaseOffset);
		}
		if (outputs[SAW_OUTPUT].active) {
			sawOut += mix * amplitude * _saw.nextFromPhasor(_phasor, phaseOffset);
		}
		if (outputs[TRIANGLE_OUTPUT].active) {
			triangleOut += mix * amplitude * _triangle.nextFromPhasor(_phasor, phaseOffset);
		}
	}

	outputs[SQUARE_OUTPUT].value = squareOut;
	outputs[SAW_OUTPUT].value = sawOut;
	outputs[TRIANGLE_OUTPUT].value = triangleOut;
	outputs[SINE_OUTPUT].value = outputs[SINE_OUTPUT].active ? (amplitude * _sine.nextFromPhasor(_phasor, phaseOffset)) : 0.0f;
}

void VCO::setSampleRate(float sampleRate) {
	_oversampleThreshold = 0.06f * sampleRate;
	_phasor.setSampleRate(sampleRate);
	_square.setSampleRate(sampleRate);
	_saw.setSampleRate(sampleRate);
	_squareDecimator.setParams(sampleRate, oversample);
	_sawDecimator.setParams(sampleRate, oversample);
	_triangleDecimator.setParams(sampleRate, oversample);
	_squarePulseWidthSL.setParams(sampleRate, 0.1f, 2.0f);
}

void VCO::setFrequency(float frequency) {
	if (_frequency != frequency && frequency < 0.475f * _phasor._sampleRate) {
		_frequency = frequency;
		_phasor.setFrequency(_frequency / (float)oversample);
		_square.setFrequency(_frequency);
		_saw.setFrequency(_frequency);
	}
}

struct VCOWidget : ModuleWidget {
	static constexpr int hp = 10;

	VCOWidget(VCO* module) : ModuleWidget(module) {
		box.size = Vec(RACK_GRID_WIDTH * hp, RACK_GRID_HEIGHT);

		{
			SVGPanel *panel = new SVGPanel();
			panel->box.size = box.size;
			panel->setBackground(SVG::load(assetPlugin(plugin, "res/VCO.svg")));
			addChild(panel);
		}

		addChild(Widget::create<ScrewSilver>(Vec(0, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 15, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(0, 365)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 15, 365)));

		// generated by svg_widgets.rb
		auto frequencyParamPosition = Vec(41.0, 45.0);
		auto fineParamPosition = Vec(48.0, 153.0);
		auto slowParamPosition = Vec(122.0, 157.2);
		auto pwParamPosition = Vec(62.0, 188.0);
		auto fmParamPosition = Vec(62.0, 230.0);
		auto fmTypeParamPosition = Vec(100.5, 231.5);

		auto pwInputPosition = Vec(15.0, 274.0);
		auto fmInputPosition = Vec(47.0, 274.0);
		auto pitchInputPosition = Vec(15.0, 318.0);
		auto syncInputPosition = Vec(47.0, 318.0);

		auto squareOutputPosition = Vec(79.0, 274.0);
		auto sawOutputPosition = Vec(111.0, 274.0);
		auto triangleOutputPosition = Vec(79.0, 318.0);
		auto sineOutputPosition = Vec(111.0, 318.0);

		auto slowLightPosition = Vec(82.0, 158.5);
		// end generated by svg_widgets.rb

		addParam(ParamWidget::create<Knob68>(frequencyParamPosition, module, VCO::FREQUENCY_PARAM, -3.0, 6.0, 0.0));
		addParam(ParamWidget::create<Knob16>(fineParamPosition, module, VCO::FINE_PARAM, -1.0, 1.0, 0.0));
		addParam(ParamWidget::create<StatefulButton9>(slowParamPosition, module, VCO::SLOW_PARAM, 0.0, 1.0, 0.0));
		addParam(ParamWidget::create<Knob26>(pwParamPosition, module, VCO::PW_PARAM, -1.0, 1.0, 0.0));
		addParam(ParamWidget::create<Knob26>(fmParamPosition, module, VCO::FM_PARAM, 0.0, 1.0, 0.0));
		addParam(ParamWidget::create<SliderSwitch2State14>(fmTypeParamPosition, module, VCO::FM_TYPE_PARAM, 0.0, 1.0, 1.0));

		addInput(Port::create<Port24>(pitchInputPosition, Port::INPUT, module, VCO::PITCH_INPUT));
		addInput(Port::create<Port24>(syncInputPosition, Port::INPUT, module, VCO::SYNC_INPUT));
		addInput(Port::create<Port24>(pwInputPosition, Port::INPUT, module, VCO::PW_INPUT));
		addInput(Port::create<Port24>(fmInputPosition, Port::INPUT, module, VCO::FM_INPUT));

		addOutput(Port::create<Port24>(squareOutputPosition, Port::OUTPUT, module, VCO::SQUARE_OUTPUT));
		addOutput(Port::create<Port24>(sawOutputPosition, Port::OUTPUT, module, VCO::SAW_OUTPUT));
		addOutput(Port::create<Port24>(triangleOutputPosition, Port::OUTPUT, module, VCO::TRIANGLE_OUTPUT));
		addOutput(Port::create<Port24>(sineOutputPosition, Port::OUTPUT, module, VCO::SINE_OUTPUT));

		addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(slowLightPosition, module, VCO::SLOW_LIGHT));
	}
};

Model* modelVCO = createModel<VCO, VCOWidget>("Bogaudio-VCO", "VCO",  "oscillator", OSCILLATOR_TAG);
