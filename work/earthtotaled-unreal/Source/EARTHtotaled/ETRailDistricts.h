#pragma once

#include "ETRing.h"

namespace ETRailDistrict
{
    enum class EShell { Shop, Hall, House, Flats };
    struct FVenue { const TCHAR* Name; EShell Shell; const TCHAR* Stock; const TCHAR* Contents; };
    struct FQuarter {const TCHAR* Landmark; const TCHAR* Yard; FColor Color; FVenue Places[2];};
    inline const FQuarter& Get(int City) {
        using S=EShell;
        static const FQuarter Quarters[]={
            {TEXT("Beacon"),TEXT("Relief yard"),{128,232,193},{{TEXT("Ranger dispatch"),S::House,TEXT("relief"),TEXT("evacuation rations")},{TEXT("Night infirmary"),S::Shop,TEXT("medical"),TEXT("triage supplies")}}},
            {TEXT("QuayCrane"),TEXT("Freight wharf"),{250,153,86},{{TEXT("Bonded freight shed"),S::Hall,TEXT("tools"),TEXT("dockworkers' tool chest")},{TEXT("Harbor chandlery"),S::Shop,TEXT("electronics"),TEXT("marine radio parts")}}},
            {TEXT("Glasshouse"),TEXT("Winter gardens"),{161,223,111},{{TEXT("Seed exchange"),S::Shop,TEXT("pantry"),TEXT("seed keepers' provisions")},{TEXT("Packing house"),S::Hall,TEXT("relief"),TEXT("harvest convoy stores")}}},
            {TEXT("Kilns"),TEXT("Potters' row"),{251,151,90},{{TEXT("Furnace hall"),S::Hall,TEXT("tools"),TEXT("kiln setters' tools")},{TEXT("Fire brigade"),S::House,TEXT("medical"),TEXT("burn treatment kit")}}},
            {TEXT("Aerial"),TEXT("Broadcast hill"),{119,204,251},{{TEXT("Last transmission"),S::Flats,TEXT("electronics"),TEXT("broadcast spares")},{TEXT("Cable exchange"),S::Shop,TEXT("tools"),TEXT("linemen's field box")}}},
            {TEXT("Vats"),TEXT("Resin works"),{253,191,74},{{TEXT("Distilling house"),S::Hall,TEXT("tools"),TEXT("valve fitter's chest")},{TEXT("Assay office"),S::Shop,TEXT("medical"),TEXT("assay laboratory stock")}}},
            {TEXT("SawWheel"),TEXT("Timber yard"),{179,212,154},{{TEXT("Blackpine sawmill"),S::Hall,TEXT("tools"),TEXT("sawyer's cabinet")},{TEXT("Bunkhouse"),S::House,TEXT("pantry"),TEXT("logging crew's larder")}}},
            {TEXT("Arch"),TEXT("Bridge market"),{218,169,149},{{TEXT("Toll office"),S::House,TEXT("armory"),TEXT("bridge guard locker")},{TEXT("Viaduct canteen"),S::Shop,TEXT("pantry"),TEXT("night shift provisions")}}},
            {TEXT("Turbine"),TEXT("Wind farm"),{174,222,235},{{TEXT("Blade repair hall"),S::Hall,TEXT("tools"),TEXT("turbine service kit")},{TEXT("Grid office"),S::Shop,TEXT("electronics"),TEXT("switchboard drawers")}}},
            {TEXT("Dish"),TEXT("Listening array"),{158,192,253},{{TEXT("Receiver house"),S::House,TEXT("electronics"),TEXT("receiver modules")},{TEXT("Field acoustics lab"),S::Shop,TEXT("medical"),TEXT("expedition medical stores")}}},
            {TEXT("Headframe"),TEXT("Pithead"),{239,171,113},{{TEXT("Winding house"),S::Hall,TEXT("tools"),TEXT("hoist mechanic's stock")},{TEXT("Pit rescue"),S::House,TEXT("relief"),TEXT("miners' rescue supplies")}}},
            {TEXT("Clock"),TEXT("Clock square"),{233,185,133},{{TEXT("Clockmakers' guild"),S::Flats,TEXT("electronics"),TEXT("clockmaker's parts")},{TEXT("Bell Street bakery"),S::Shop,TEXT("pantry"),TEXT("baker's cupboard")}}},
            {TEXT("FilterBeds"),TEXT("Filtration works"),{132,221,207},{{TEXT("Filter house"),S::Hall,TEXT("relief"),TEXT("water crew stores")},{TEXT("Pump dispensary"),S::Shop,TEXT("medical"),TEXT("dispensary cabinet")}}},
            {TEXT("CanopyWalk"),TEXT("Forest observatory"),{156,217,138},{{TEXT("Survey lodge"),S::House,TEXT("armory"),TEXT("rangers' locker")},{TEXT("Canopy field school"),S::Shop,TEXT("relief"),TEXT("survey team supplies")}}},
            {TEXT("Orrery"),TEXT("Astronomers' court"),{174,168,248},{{TEXT("Lens workshop"),S::Shop,TEXT("electronics"),TEXT("telescope components")},{TEXT("Star atlas library"),S::Flats,TEXT("relief"),TEXT("archive emergency stores")}}},
            {TEXT("SaltSilos"),TEXT("Salt exchange"),{235,216,162},{{TEXT("Weigh house"),S::House,TEXT("tools"),TEXT("scale fitter's chest")},{TEXT("Pickling hall"),S::Hall,TEXT("pantry"),TEXT("preserved market stock")}}}
        };
        return Quarters[City];
    }
    inline FVector2D Lot(int City,int Place) {
        const auto Block=FVector2D(ETNav::District(ETRing::CityStation(City)))*ETNav::DistrictSpacing;
        return Block+FVector2D(Place?1220.:-2150.,-12800.);
    }
    inline double Front(EShell S) {return S==EShell::Hall?900.:S==EShell::Flats?600.:505.;}
    struct FStreet { const TCHAR* Name; const TCHAR* Fixture; const TCHAR* Material; double Scale; int Spacing; };
    inline const FStreet& Street(int City) {
        static const FStreet Styles[]={
            {TEXT("Evacuation Way"),TEXT("CampShelter"),TEXT("Canvas"),.85,3},
            {TEXT("Gantry Street"),TEXT("QuarterQuayCrane"),TEXT("Steel"),.62,4},
            {TEXT("Glasshouse Walk"),TEXT("QuarterGlasshouse"),TEXT("Greenline"),.72,3},
            {TEXT("Potters' Lane"),TEXT("QuarterKilns"),TEXT("Brick"),.67,4},
            {TEXT("Antenna Row"),TEXT("Relay"),TEXT("Steel"),1.4,3},
            {TEXT("Varnish Street"),TEXT("QuarterVats"),TEXT("Rust"),.82,4},
            {TEXT("Sawmill Road"),TEXT("FallenLog"),TEXT("Bark"),1.1,2},
            {TEXT("Tollgate Row"),TEXT("Canopy"),TEXT("Canvas"),.78,4},
            {TEXT("Turbine Way"),TEXT("QuarterTurbine"),TEXT("Steel"),.74,4},
            {TEXT("Receiver Street"),TEXT("QuarterDish"),TEXT("Steel"),.59,3},
            {TEXT("Colliery Lane"),TEXT("QuarterHeadframe"),TEXT("Rust"),.70,4},
            {TEXT("Belfry Street"),TEXT("Bench"),TEXT("Wood"),1.1,2},
            {TEXT("Sluice Street"),TEXT("WaterTank"),TEXT("Greenline"),1.3,3},
            {TEXT("Crown Walk"),TEXT("QuarterCanopyWalk"),TEXT("Wood"),.87,3},
            {TEXT("Transit of Venus Way"),TEXT("QuarterDish"),TEXT("Steel"),.64,4},
            {TEXT("Saltworkers' Row"),TEXT("QuarterSaltSilos"),TEXT("Plaster"),.71,4}
        };
        return Styles[City];
    }
    inline FString CacheId(int City,int Place) {return FString::Printf(TEXT("rail-quarter/%d/%d"),City,Place);}
    inline FVector2D Cache(int City,int Place) {return Lot(City,Place)+FVector2D(0.,-130.);}
    inline FString PinLabel(FVector2D XY,int Index) {
        const int City=ETRing::CityIn(ETNav::District(XY));
        if(City!=INDEX_NONE) {
            for(int I:{0,1}) if(XY.Equals(Lot(City,I),1.)) return Get(City).Places[I].Name;
            if(XY.Equals(ETRing::CityStation(City),1.)) return FString(ETRing::CityName(City))+TEXT(" station");
            const int Link=ETRing::Connection(City);
            if(Link!=INDEX_NONE&&XY.Equals(ETRing::Dock(Link,City<8?0:1),1.)) return ETRing::ServiceName(Link);
            for(int Line:{City/ETRing::Stops,Link}) if(Line!=INDEX_NONE) {
                const int Stop=ETRing::Shuttle(Line)?(City<8?0:1):City%ETRing::Stops;
                if(XY.Equals(ETRing::LiftPoint(Line,Stop),1.)) return FString(ETRing::ServiceName(Line))+TEXT(" street lift");
            }
        }
        return FString::Printf(TEXT("Pin %d"),Index+1);
    }
}
