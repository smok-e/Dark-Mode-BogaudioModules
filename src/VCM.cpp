
#include "VCM.hpp"

void VCM::step() {
	bool linear = lights[LINEAR_LIGHT].value = params[LINEAR_PARAM].value > 0.5f;
	if (outputs[MIX_OUTPUT].active) {
		float out = channelStep(inputs[IN1_INPUT], params[LEVEL1_PARAM], inputs[CV1_INPUT], _amplifier1, linear);
		out += channelStep(inputs[IN2_INPUT], params[LEVEL2_PARAM], inputs[CV2_INPUT], _amplifier2, linear);
		out += channelStep(inputs[IN3_INPUT], params[LEVEL3_PARAM], inputs[CV3_INPUT], _amplifier3, linear);
		out += channelStep(inputs[IN4_INPUT], params[LEVEL4_PARAM], inputs[CV4_INPUT], _amplifier4, linear);
		float level = params[MIX_PARAM].value;
		if (inputs[MIX_CV_INPUT].active) {
			level *= clamp(inputs[MIX_CV_INPUT].value / 10.0f, 0.0f, 1.0f);
		}
		outputs[MIX_OUTPUT].value = level * out;
	}
}

float VCM::channelStep(Input& input, Param& knob, Input& cv, Amplifier& amplifier, bool linear) {
	if (!input.active) {
		return 0.0f;
	}
	float level = knob.value;
	if (cv.active) {
		level *= clamp(cv.value / 10.0f, 0.0f, 1.0f);
	}
	if (linear) {
		return level * input.value;
	}
	level = 1.0f - level;
	level *= Amplifier::minDecibels;
	amplifier.setLevel(level);
	return amplifier.next(input.value);
}

struct VCMWidget : ModuleWidget {
	VCMWidget(VCM* module) : ModuleWidget(module) {
		box.size = Vec(RACK_GRID_WIDTH * 10, RACK_GRID_HEIGHT);

		{
			SVGPanel *panel = new SVGPanel();
			panel->box.size = box.size;
			panel->setBackground(SVG::load(assetPlugin(plugin, "res/VCM.svg")));
			addChild(panel);
		}

		addChild(Widget::create<ScrewSilver>(Vec(0, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 15, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(0, 365)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 15, 365)));

		// generated by svg_widgets.rb
		auto level1ParamPosition = Vec(89.5, 35.5);
		auto level2ParamPosition = Vec(89.5, 99.5);
		auto level3ParamPosition = Vec(89.5, 163.5);
		auto level4ParamPosition = Vec(89.5, 228.5);
		auto mixParamPosition = Vec(22.5, 293.5);
		auto linearParamPosition = Vec(95.0, 342.7);

		auto in1InputPosition = Vec(14.0, 37.0);
		auto cv1InputPosition = Vec(45.0, 37.0);
		auto in2InputPosition = Vec(14.0, 101.0);
		auto cv2InputPosition = Vec(45.0, 101.0);
		auto in3InputPosition = Vec(14.0, 165.0);
		auto cv3InputPosition = Vec(45.0, 165.0);
		auto in4InputPosition = Vec(14.0, 230.0);
		auto cv4InputPosition = Vec(45.0, 230.0);
		auto mixCvInputPosition = Vec(81.0, 294.0);

		auto mixOutputPosition = Vec(112.0, 294.0);

		auto linearLightPosition = Vec(46.0, 344.0);
		// end generated by svg_widgets.rb

		addParam(ParamWidget::create<Knob38>(level1ParamPosition, module, VCM::LEVEL1_PARAM, 0.0, 1.0, 0.8));
		addParam(ParamWidget::create<Knob38>(level2ParamPosition, module, VCM::LEVEL2_PARAM, 0.0, 1.0, 0.8));
		addParam(ParamWidget::create<Knob38>(level3ParamPosition, module, VCM::LEVEL3_PARAM, 0.0, 1.0, 0.8));
		addParam(ParamWidget::create<Knob38>(level4ParamPosition, module, VCM::LEVEL4_PARAM, 0.0, 1.0, 0.8));
		addParam(ParamWidget::create<Knob38>(mixParamPosition, module, VCM::MIX_PARAM, 0.0, 1.0, 0.8));
		addParam(ParamWidget::create<StatefulButton9>(linearParamPosition, module, VCM::LINEAR_PARAM, 0.0, 1.0, 0.0));

		addInput(Port::create<Port24>(in1InputPosition, Port::INPUT, module, VCM::IN1_INPUT));
		addInput(Port::create<Port24>(cv1InputPosition, Port::INPUT, module, VCM::CV1_INPUT));
		addInput(Port::create<Port24>(in2InputPosition, Port::INPUT, module, VCM::IN2_INPUT));
		addInput(Port::create<Port24>(cv2InputPosition, Port::INPUT, module, VCM::CV2_INPUT));
		addInput(Port::create<Port24>(in3InputPosition, Port::INPUT, module, VCM::IN3_INPUT));
		addInput(Port::create<Port24>(cv3InputPosition, Port::INPUT, module, VCM::CV3_INPUT));
		addInput(Port::create<Port24>(in4InputPosition, Port::INPUT, module, VCM::IN4_INPUT));
		addInput(Port::create<Port24>(cv4InputPosition, Port::INPUT, module, VCM::CV4_INPUT));
		addInput(Port::create<Port24>(mixCvInputPosition, Port::INPUT, module, VCM::MIX_CV_INPUT));

		addOutput(Port::create<Port24>(mixOutputPosition, Port::OUTPUT, module, VCM::MIX_OUTPUT));

		addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(linearLightPosition, module, VCM::LINEAR_LIGHT));
	}
};

Model* modelVCM = Model::create<VCM, VCMWidget>("Bogaudio", "Bogaudio-VCM", "VCM");
