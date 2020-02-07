#include <ns/ns_arguments.hpp>

constexpr const char NeutronStarArguments::default_nsprop[];
constexpr const double NeutronStarArguments::default_Bx;
constexpr const double NeutronStarArguments::default_hotspotarea;
constexpr const double NeutronStarArguments::default_epsilonAlfven;
constexpr const double NeutronStarArguments::default_inversebeta;
constexpr const double NeutronStarArguments::default_Rdead;
constexpr const char NeutronStarArguments::default_fptype[];
constexpr const double NeutronStarArguments::default_Rx_dummy;
constexpr const double NeutronStarArguments::default_freqx_dummy;

double NeutronStarArguments::initializeFreqx(const std::string& nsprop) {
	if (nsprop == "dummy") {
		return default_freqx_dummy;
	}
	if (nsprop == "sibgatullinsunyaev2000" || nsprop == "sibsun2000") {
		throw std::runtime_error("freqx must be specified for nsprop=sibgatullinsunyaev2000");
	}
	throw std::invalid_argument("Wrong nsprop value");
}

double NeutronStarArguments::initializeRx(const std::string& nsprop, const std::optional<double> freqx) {
	if (nsprop == "dummy") {
		return default_Rx_dummy;
	}
	if (nsprop == "sibgatullinsunyaev2000" || nsprop == "sibsun2000") {
		if (!freqx) {
			throw std::runtime_error("freqx must be specified for nsprop=sibgatullinsunyaev2000");
		}
		return SibgatullinSunyaev2000Geometry::radiusNS(*freqx);
	}
	throw std::invalid_argument("Wrong nsprop value");
}


double SibgatullinSunyaev2000Geometry::radiusNS(double freqx) {
	const double freqx_kHz = freqx / 1000.0;
	// Eq. 25
	const double r_km = 12.44 - 3.061 * freqx_kHz + 0.843 * m::pow<2>(freqx_kHz) + 0.6 * m::pow<3>(freqx_kHz)
	        + 1.56 * m::pow<4>(freqx_kHz);
	return 1e5 * r_km;
}

double SibgatullinSunyaev2000Geometry::radiusISCO(double freqx) {
	const double freqx_kHz = freqx / 1000.0;
	// Eq. 3, Eq. 26
	const double isco_minus_ns_km = 1.44 - 3.061 * freqx_kHz + 0.843 * m::pow<2>(freqx_kHz) + 0.6 * m::pow<3>(freqx_kHz) -
								 	0.22 * m::pow<4>(freqx_kHz);
	return 1e5 * isco_minus_ns_km + radiusNS(freqx);
}


std::optional<double> NeutronStarBasicDiskBinaryArguments::initializeRisco(const NeutronStarArguments& ns_args, std::optional<double> risco){
	if (risco) {
		return risco;
	}
	if (ns_args.nsprop == "dummy") {
		return risco;
	}
	if (ns_args.nsprop == "sibgatullinsunyaev2000" || ns_args.nsprop == "sibsun2000") {
		return SibgatullinSunyaev2000Geometry::radiusISCO(ns_args.freqx);
	}
	throw std::invalid_argument("Wrong nsprop value");
}
