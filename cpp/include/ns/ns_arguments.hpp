#ifndef FREDDI_NS_ARGUMENTS_HPP
#define FREDDI_NS_ARGUMENTS_HPP

#include <optional>
#include <string>

#include <arguments.hpp>
#include <util.hpp>

namespace SibgatullinSunyaev2000Geometry {
	static double radiusNS(double freqx);
	static double radiusISCO(double reqx);
};

class NeutronStarArguments {
public:
	constexpr static const char default_nsprop[] = "dummy";
	constexpr static const double default_Bx = 0.;
	constexpr static const double default_hotspotarea = 1.;
	constexpr static const double default_epsilonAlfven = 1.;
	constexpr static const double default_inversebeta = 0.;
	constexpr static const double default_Rdead = 0.;
	constexpr static const char default_fptype[] = "no-outflow";
protected:
	constexpr static const double default_Rx_dummy = 1e6;
	constexpr static const double default_freqx_dummy = 0.;
public:
	std::string nsprop;
	double freqx;
	double Rx;
	double Bx;
	double hotspotarea;
	double epsilonAlfven;
	double inversebeta;
	double Rdead;
	std::string fptype;
	pard fpparams;
protected:
	static double initializeFreqx(const std::string& nsprop);
	static double initializeRx(const std::string& nsprop, std::optional<double> freqx);
public:
	NeutronStarArguments(
			const std::string& nsprop,
			std::optional<double> freqx,
			std::optional<double> Rx,
			double Bx, double hotspotarea,
			double epsilonAlfven, double inversebeta, double Rdead,
			const std::string& fptype, const pard& fpparams):
			nsprop(nsprop),
			freqx(freqx ? *freqx : initializeFreqx(nsprop)),
			Rx(Rx ? *Rx : initializeRx(nsprop, freqx)),
			Bx(Bx), hotspotarea(hotspotarea),
			epsilonAlfven(epsilonAlfven), inversebeta(inversebeta), Rdead(Rdead),
			fptype(fptype), fpparams(fpparams) {}
};



class NeutronStarBasicDiskBinaryArguments: public BasicDiskBinaryArguments {
protected:
	static std::optional<double> initializeRisco(const NeutronStarArguments& ns_args, std::optional<double> risco);
public:
	NeutronStarBasicDiskBinaryArguments(
			const NeutronStarArguments& ns_args,
			double alpha,
			double Mx, double kerr,
			double period,
			double Mopt, std::optional<double> Ropt, double Topt,
			std::optional<double> rin, std::optional<double> rout, std::optional<double> risco
	):
			BasicDiskBinaryArguments(
				alpha,
				Mx, kerr,
				period,
				Mopt,
				Ropt,
				Topt,
				rin,
				rout,
				initializeRisco(ns_args, risco)) {}
};


class FreddiNeutronStarArguments: public FreddiArguments {
public:
	std::shared_ptr<NeutronStarArguments> ns;
public:
	FreddiNeutronStarArguments() = default;
	FreddiNeutronStarArguments(const FreddiArguments& freddi_args, NeutronStarArguments* ns):
			FreddiArguments(freddi_args), ns(ns) {}
	FreddiNeutronStarArguments(
			GeneralArguments* general,
			NeutronStarBasicDiskBinaryArguments* basic,
			DiskStructureArguments* disk,
			SelfIrradiationArguments* irr,
			FluxArguments* flux,
			CalculationArguments* calc,
			NeutronStarArguments* ns):
			FreddiArguments(general, basic, disk, irr, flux, calc), ns(ns) {}
};


#endif //FREDDI_NS_ARGUMENTS_HPP
