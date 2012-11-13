#!/usr/bin/env python
from openravepy import *
RaveInitialize()
RaveLoadPlugin('build/ravespacenav')
try:
    env=Environment()
    env.Load('scenes/myscene.env.xml')
    SpaceNav = RaveCreateModule(env,'SpaceNav')
    print SpaceNav.SendCommand('help')
finally:
    RaveDestroy()
