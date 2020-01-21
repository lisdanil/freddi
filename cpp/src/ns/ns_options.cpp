#include <ns/ns_options.hpp>

NeutronStarOptions::NeutronStarOptions(const po::variables_map &vm):
		NeutronStarArguments(
				vm["Rx"].as<double>(),
				vm["freqx"].as<double>(),
				vm["Bx"].as<double>(),
				vm["hotspotarea"].as<double>(),
				vm["epsilonAlfven"].as<double>(),
				vm["inversebeta"].as<double>(),
				vm["Rdead"].as<double>(),
				vm["fptype"].as<std::string>(),
				fpparamsInitializer(vm)) {}

pard NeutronStarOptions::fpparamsInitializer(const po::variables_map& vm) {
	pard m;
	if (vm.count("fpparams") == 0) {
		return m;
	}
	const std::vector<std::string > tokens(vm["fpparams"].as<std::vector<std::string> >());
	for (const auto& token : tokens) {
		std::vector<std::string> parts;
		boost::split(parts, token, boost::is_any_of(":"));
		if (parts.size() != 2) {
			throw po::validation_error(po::validation_error::invalid_option_value);
		}
		m[parts[0]] = std::stod(parts[1]);
	}
	return m;
}

po::options_description NeutronStarOptions::description() {
	po::options_description od("Parameters of accreting neutron star");
	od.add_options()
			( "Rx", po::value<double>()->default_value(default_Rx), "Accretor radius, cm" )
			( "freqx", po::value<double>()->default_value(default_freqx), "Accretor rotation frequency, Hz. This parameter is not linked to --kerr, agree them yourself" )
			( "Bx", po::value<double>()->default_value(default_Bx), "Accretor polar magnetic induction, G" )
			( "hotspotarea", po::value<double>()->default_value(default_hotspotarea), "??? Relative area of hot spot on the accretor" )
			( "epsilonAlfven", po::value<double>()->default_value(default_epsilonAlfven), "Factor in Alfven radius formula" )
			( "inversebeta", po::value<double>()->default_value(default_inversebeta), "???" )
			( "Rdead", po::value<double>()->default_value(default_Rdead), "Maximum inner radius of the disk that can be obtained, it characterises minimum torque in the dead disk, cm" )
			( "fptype", po::value<std::string>()->default_value(default_fptype), "Accretor Mdot fraction mode: no-outflow, propeller, corotation-block, eksi-kultu2010, romanova2018, geometrical" )
			( "fpparams", po::value<std::vector<std::string> >()->multitoken(), "Accretor Mdot fraction parameters, specific for each fptype. Format is name:value. Examples: 1) for geometrical chi:15; 2) for romanova2018 par1:0.15 par2:0.92" )
			;
	return od;
}


FreddiNeutronStarOptions::FreddiNeutronStarOptions(const po::variables_map &vm) {
	general.reset(new GeneralOptions(vm));
	basic.reset(new BasicDiskBinaryOptions(vm));
	disk.reset(new DiskStructureOptions(vm, *basic));
	irr.reset(new SelfIrradiationOptions(vm, *disk));
	flux.reset(new FluxOptions(vm));
	calc.reset(new CalculationOptions(vm));
	ns.reset(new NeutronStarOptions(vm));
}

po::options_description FreddiNeutronStarOptions::description() {
	po::options_description desc("Freddi NS: numerical calculation of accretion disk evolution");
	desc.add(GeneralOptions::description());
	desc.add(BasicDiskBinaryOptions::description());
	desc.add(DiskStructureOptions::description());
	desc.add(NeutronStarOptions::description());
	desc.add(SelfIrradiationOptions::description());
	desc.add(FluxOptions::description());
	desc.add(CalculationOptions::description());
	return desc;
}
