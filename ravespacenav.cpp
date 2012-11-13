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
}

