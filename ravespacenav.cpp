/*
Copyright (c) 2012, Robert W. Ellenberg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the names of its contributors may 
      be used to endorse or promote products derived from this software 
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, 
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <openrave/plugin.h>
#include <boost/bind.hpp>
#include <spnav.h>

using namespace OpenRAVE;

struct spnav_state {
    int x, y, z;
    int rx, ry, rz;
    bool b0,b1;
};

class SpaceNav : public ModuleBase
{
    public:
        SpaceNav(EnvironmentBasePtr penv, std::istream& ss) : ModuleBase(penv) {
            RegisterCommand("OpenSpaceNav",boost::bind(&SpaceNav::OpenSpaceNav,this,_1,_2),
                    "Manually open the connection to the spacenav");
            RegisterCommand("CloseSpaceNav",boost::bind(&SpaceNav::CloseSpaceNav,this,_1,_2),
                    "Manually Close connection to the spacenav");
            RegisterCommand("GetMotion",boost::bind(&SpaceNav::GetMotion,this,_1,_2),
                    "Get most recent motion event");
            RegisterCommand("GetButton",boost::bind(&SpaceNav::GetButton,this,_1,_2),
                    "Get most recent button event");
            RegisterCommand("GetState",boost::bind(&SpaceNav::GetState,this,_1,_2),
                    "Get current joystick state");
            if(spnav_open()==-1) {
                RAVELOG_ERROR("failed to connect to the space navigator daemon\n");
                _running=false;
            }
            else _running=true;
            _state.x=0;
            _state.y=0;
            _state.z=0;
            _state.rx=0;
            _state.ry=0;
            _state.rz=0;
            _state.b0=0;
            _state.b1=0;
            _button.press=0;
            _button.bnum=0;
        }

        virtual ~SpaceNav() {
            spnav_close();
        }

        bool OpenSpaceNav(std::ostream& sout, std::istream& sinput)
        {
            int result = spnav_open();
            sout << result;
            if (result == -1) return false;
            return true;
        }

        bool CloseSpaceNav(std::ostream& sout, std::istream& sinput)
        {
            int result = spnav_close();
            sout << result;
            if (result == -1) return false;
            return true;
        }

        bool GetMotion(std::ostream& sout, std::istream& sinput)
        {
            sout << _motion.x << " " << _motion.y << " " << _motion.z << " " 
                << _motion.rx << " " << _motion.ry << " " << _motion.rz << std::endl;
            return true;
        }

        bool GetButton(std::ostream& sout, std::istream& sinput)
        {
            sout << _button.press << " " << _button.bnum << std::endl;
            return true;
        }

        bool GetState(std::ostream& sout, std::istream& sinput)
        {
            sout << _state.x << " " << _state.y << " " << _state.z << " " 
                << _state.rx << " " << _state.ry << " " << _state.rz << " " << _state.b0 << " " << _state.b1 << std::endl;
            return true;
        }

        
        /**
         * Process spnav events that have elapsed.
         * Events will accumulate from the spacenavigator that must be
         * incrementally processed to determine the current state.
         */
        virtual bool SimulationStep(dReal elapsedTime) {
            spnav_event sev;
            if (_running){
                //Loop over all captured events to update the state. Need to do this because button release events can be missed.
                while(spnav_poll_event(&sev)) {
                    if(sev.type == SPNAV_EVENT_MOTION) {

                        RAVELOG_VERBOSE("got motion event: t(%d, %d, %d), r(%d, %d, %d)\n", sev.motion.x, sev.motion.y, sev.motion.z, sev.motion.rx, sev.motion.ry, sev.motion.rz);

                        _motion=sev.motion;
                    } else {	/* SPNAV_EVENT_BUTTON */
                        RAVELOG_VERBOSE("got button %s event b(%d)\n", sev.button.press ? "press" : "release", sev.button.bnum);
                        _button=sev.button;
                    }
                    UpdateState();
                }
            }
            return true;
        };

        const spnav_state& GetState() const {
            return _state;
        }

        
    protected:
        void UpdateState(){
            _state.x=_motion.x;
            _state.y=_motion.y;
            _state.z=_motion.z;
            _state.rx=_motion.rx;
            _state.ry=_motion.ry;
            _state.rz=_motion.rz;
            switch (_button.bnum){
                case 0:
                    _state.b0=_button.press;
                    break;
                case 1:
                    _state.b1=_button.press;
                    break;
            }

        }
    private:

        bool _running;
        spnav_event_motion _motion;
        spnav_event_button _button;

        /** Overall internal state of spacenav */
        spnav_state _state;
};


// called to create a new plugin
InterfaceBasePtr CreateInterfaceValidated(InterfaceType type, const std::string& interfacename, std::istream& sinput, EnvironmentBasePtr penv)
{
    if( type == PT_Module && interfacename == "spacenav" ) {
        return InterfaceBasePtr(new SpaceNav(penv,sinput));
    }

    return InterfaceBasePtr();
}

// called to query available plugins
void GetPluginAttributesValidated(PLUGININFO& info)
{
    info.interfacenames[PT_Module].push_back("SpaceNav");
}

// called before plugin is terminated
OPENRAVE_PLUGIN_API void DestroyPlugin()
{
    //TODO: spacenav cleanup here?
}

