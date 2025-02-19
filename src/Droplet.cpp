
#include "plugin.hpp"
#include "Common.hpp"
#include "Droplet.hpp"

using namespace prism;
using namespace droplet;

struct Droplet;

struct Droplet : core::PrismModule {

	enum ParamIds {
		Q_PARAM,
		FREQ_PARAM,
		FILTER_PARAM,
		ENV_PARAM,
		NOISE_PARAM,
		FREQ_ATTN_PARAM,
		Q_ATTN_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		Q_INPUT,
		FREQ_INPUT,
		IN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		ENV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	Filter		filter;
	IO 			io;
	Audio 		audio;

	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {}

	Droplet() : core::PrismModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) { 
		configParam(Q_PARAM, 0.0, 10.0, 5.0, "Q");
		configParam(FREQ_PARAM, -10.0, 10.0, 0, "Filter frequency");
		configParam(Q_ATTN_PARAM, 0.0, 1.0, 1.0, "Q input attenuation");
		configParam(FREQ_ATTN_PARAM, 0.0, 1.0, 1.0, "Frequency input attenuation");

		configSwitch(FILTER_PARAM, 0, 1, 0, "Filter type", {"2-pass", "1-pass"});
		configSwitch(ENV_PARAM, 0, 2, 0, "Envelope", {"fast", "slow", "trigger"});
		configSwitch(NOISE_PARAM, 0, 2, 0, "Noise", {"brown", "pink", "white"});

		configInput(Q_INPUT, "Q CV");
		configInput(FREQ_INPUT, "Frequency CV");
		configInput(IN_INPUT, "Audio");
		configOutput(OUT_OUTPUT, "Audio");
		configOutput(ENV_OUTPUT, "Envelope");

		configBypass(IN_INPUT, OUT_OUTPUT);

		filter.configure(&io);
		filter.initialise();
	}

	void onReset() override {
		filter.initialise();
	}

	void process(const ProcessArgs &args) override;
};

void Droplet::process(const ProcessArgs &args) {
	if (inputs[IN_INPUT].isConnected()) {

		PrismModule::step();

		io.FILTER_SWITCH	= (FilterSetting)params[FILTER_PARAM].getValue();

		float q 			= inputs[Q_INPUT].getVoltage() * params[Q_ATTN_PARAM].getValue();
		io.Q_LEVEL			= (int16_t)clamp(q * 409.5f, 0.0f, 4095.0f);

		io.Q_CONTROL		= (int16_t)params[Q_PARAM].getValue() * 409.5f;

		float f 			= inputs[FREQ_INPUT].getVoltage() * params[FREQ_ATTN_PARAM].getValue();
		io.FREQ 			= dsp::FREQ_C4 * pow(2.0f, clamp(f + params[FREQ_PARAM].getValue(), -10.0f, 10.0f));

		io.ENV_SWITCH		= (EnvelopeMode)params[ENV_PARAM].getValue();

		audio.noiseSelected 	= params[NOISE_PARAM].getValue();;
		audio.sampleRate 		= args.sampleRate;
		
		audio.ChannelProcess(io, inputs[IN_INPUT], outputs[OUT_OUTPUT], filter);

		// Populate poly outputs
		outputs[ENV_OUTPUT].setChannels(1);
		outputs[ENV_OUTPUT].setVoltage(clamp(io.env_out * 100.0f, 0.0f, 10.0f));
	}

}

struct DropletWidget : ModuleWidget {
	
	DropletWidget(Droplet *module) {

		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/prism_Droplet.svg")));

		addParam(createParamCentered<gui::PrismSSwitch3>(Vec(107.72, 380.0 - 276.00 - 14.005), module, Droplet::NOISE_PARAM));
		addParam(createParamCentered<gui::PrismSSwitch3>(Vec(107.72, 380.0f - 150.0f - 40.0f), module, Droplet::ENV_PARAM));
		addParam(createParamCentered<gui::PrismSSwitch>(Vec(27.72, 380.0 - 276.00 - 14.005), module, Droplet::FILTER_PARAM));

		addParam(createParamCentered<gui::PrismLargeKnobNoSnap>(Vec(44.500, 380.0f - 80.000 - 17.000), module, Droplet::Q_PARAM));
		addParam(createParamCentered<gui::PrismLargeKnobNoSnap>(Vec(90.500, 380.0f - 80.000 - 17.000), module, Droplet::FREQ_PARAM));

		addParam(createParamCentered<gui::PrismKnobNoSnap>(Vec(19.0, 380.0f - 50.000 - 17.000), module, Droplet::Q_ATTN_PARAM));
		addParam(createParamCentered<gui::PrismKnobNoSnap>(Vec(116.2, 380.0f - 50.000 - 17.000), module, Droplet::FREQ_ATTN_PARAM));

		addInput(createInputCentered<gui::PrismPort>(Vec(44.500, 380.0f - 26.000 - 11.0), module, Droplet::Q_INPUT));
		addInput(createInputCentered<gui::PrismPort>(Vec(90.500, 380.0f - 26.000 - 11.0), module, Droplet::FREQ_INPUT));

		addInput(createInputCentered<gui::PrismPort>(Vec(67.500, 380.0f - 240.000 - 11.0), module, Droplet::IN_INPUT));
		addOutput(createOutputCentered<gui::PrismPort>(Vec(67.500, 380.0f - 318.000 - 11.0), module, Droplet::OUT_OUTPUT));

		addOutput(createOutputCentered<gui::PrismPort>(Vec(67.500, 380.0f - 150.000 - 9.000), module, Droplet::ENV_OUTPUT));
	}
};

Model *modelDroplet = createModel<Droplet, DropletWidget>("Droplet");
