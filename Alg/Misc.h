#ifndef _Misc_h_
#define _Misc_h_

#include "TMatrixD.h"
#include <sstream>

///////////////////////////////////////////////////////////////////////////////////////////////

inline double GetNuMIAngle(double px, double py, double pz, std::string direction="beam"){

    // Variables
    TRotation RotDet2Beam;             // Rotations
    TVector3  detxyz, BeamCoords;      // Translations
    std::vector<double> rotmatrix;     // Inputs

    // input detector coordinates to translate
    detxyz = {px, py, pz};     

    // From beam to detector rotation matrix
    rotmatrix = {
        0.92103853804025681562, 0.022713504803924120662, 0.38880857519374290021,
        4.6254001262154668408e-05, 0.99829162468141474651, -0.058427989452906302359,
        -0.38947144863934973769, 0.053832413938664107345, 0.91946400794392302291 };

    // Return the TRotation
    TVector3 newX, newY, newZ;
    newX = TVector3(rotmatrix[0], rotmatrix[1], rotmatrix[2]);
    newY = TVector3(rotmatrix[3], rotmatrix[4], rotmatrix[5]);
    newZ = TVector3(rotmatrix[6], rotmatrix[7], rotmatrix[8]);

    RotDet2Beam.RotateAxes(newX, newY, newZ); // Return the TRotation now det to beam

    // Rotate to beam coords
    BeamCoords = RotDet2Beam * detxyz;

    TVector3 beamdir = {0 , 0 , 1};;
    
    // Get the angle wrt to the beam
    if (direction == "beam") beamdir = {0 , 0 , 1};
    
    // Get the angle wrt to the target to detector direction
    else if (direction == "target") {
        beamdir = {5502, 7259, 67270};
        beamdir = beamdir.Unit(); // Get the direction
    }
    else {
        std::cout << "Warning unknown angle type specified, you should check this" << std::endl;
    }
    
    double angle = BeamCoords.Angle(beamdir) * 180 / 3.1415926;


    // Create vectors to get the angle in the yz and xz planes
    TVector3 BeamCoords_yz = { 0, BeamCoords.Y(), BeamCoords.Z() }; // Angle upwards
    TVector3 BeamCoords_xz = { BeamCoords.X(), 0, BeamCoords.Z() }; // Angle across

    // if (theta > 50 ) std::cout <<"Theta: " << theta << "   UP: " << BeamCoords_yz.Angle(beam_dir) * 180 / 3.1415926 << "  Across: " << BeamCoords_xz.Angle(beam_dir) * 180 / 3.1415926 << std::endl;


    return angle;

}

///////////////////////////////////////////////////////////////////////////////////////////////

inline TVector3 RotateToBeam(TVector3 V){

   // Rotation from detector coords to beam coords
   TMatrixD RotToBeam(3,3);

   RotToBeam[0][0] = 0.92103853804025681562;  RotToBeam[0][1] = 4.6254001262154668408e-05; RotToBeam[0][2] = -0.38947144863934973769;
   RotToBeam[1][0] = 0.022713504803924120662; RotToBeam[1][1] = 0.99829162468141474651;    RotToBeam[1][2] = 0.053832413938664107345;
   RotToBeam[2][0] = 0.38880857519374290021;  RotToBeam[2][1] = -0.058427989452906302359;  RotToBeam[2][2] = 0.91946400794392302291;  

   TVector3 V_Beam;
   V_Beam.SetX(V.X()*RotToBeam[0][0] + V.Y()*RotToBeam[0][1] + V.Z()*RotToBeam[0][2]);
   V_Beam.SetY(V.X()*RotToBeam[1][0] + V.Y()*RotToBeam[1][1] + V.Z()*RotToBeam[1][2]);
   V_Beam.SetZ(V.X()*RotToBeam[2][0] + V.Y()*RotToBeam[2][1] + V.Z()*RotToBeam[2][2]);
  
   return V_Beam;

}

///////////////////////////////////////////////////////////////////////////////////////////////

inline double Limit(double value , std::pair<double,double> limits){

   value = std::max(value,limits.first);
   value = std::min(value,limits.second);

   return value;

}

///////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return out.str();
}

///////////////////////////////////////////////////////////////////////////////////////////////

#endif
