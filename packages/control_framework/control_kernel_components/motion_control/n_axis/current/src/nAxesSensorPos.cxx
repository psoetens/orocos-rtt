// $Id: nAxisGeneratorCartesianPos.hpp,v 1.1.1.1 2003/12/02 20:32:06 kgadeyne Exp $
// Copyright (C) 2003 Klaas Gadeyne <klaas.gadeyne@mech.kuleuven.ac.be>
//                    Wim Meeussen  <wim.meeussen@mech.kuleuven.ac.be>
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//  
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//  
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//  

#include "control_kernel/nAxesSensorPos.hpp"
#include <assert.h>

namespace ORO_ControlKernel
{

  using namespace ORO_ControlKernel;
  using namespace ORO_DeviceInterface;


  nAxesSensorPos::nAxesSensorPos(unsigned int num_axes, 
				 std::vector<AxisInterface*> axes,
				 std::string name)
    : nAxesSensorPos_typedef(name),
      _num_axes(num_axes), 
      _axes(axes),
      _position_local(num_axes),
      _position_sensors(num_axes)
  {
    assert(_axes.size() == num_axes);

    // get position sensors from axes
    for (unsigned int i=0; i<_num_axes; i++){
      _position_sensors[i] = _axes[i]->getSensor("Position");
      assert( _position_sensors[i] != NULL );
    }
  }


  nAxesSensorPos::~nAxesSensorPos(){};
  

  void nAxesSensorPos::pull()
  {
    // copy values from position sensors to local variable
    for (unsigned int i=0; i<_num_axes; i++)
      _position_local[i] = _position_sensors[i]->readSensor();
  }



  void nAxesSensorPos::calculate()
  {}


  
  void nAxesSensorPos::push()      
  {
    _position_DOI->Set(_position_local);
  }



  bool nAxesSensorPos::componentLoaded()
  {
    // get interface to Input data types
    if (!Input::dObj()->Get("Position", _position_DOI)){
      cerr << "nAxesSensorPos::componentLoaded() DataObjectInterface not found" << endl;
      return false;
    }

    // set empty values
    vector<double> _temp_vector(_num_axes);
    _position_DOI->Set(_temp_vector);

    return true;
  }


  bool nAxesSensorPos::componentStartup()
  {
    return true;
  }
  
  

} // end namespace ORO_nAxesControlKernel
