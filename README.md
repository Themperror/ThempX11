# ThempX11
ThempX11 Rendering Framework

This engine's (if you can call it that) goal is to be seen from a tech-demo point of view, not as a usable game engine. 
There's not enough optimisations in terms of RAM/VRAM usage for this to be usable by a lot of people. 
The fact I'm using Single-Pass Render to Cubemap for the point lights made this framework unsupported by anything lower than a GTX900 series
I'm also going wild on the textures and shadow maps so expect a 6GB VRAM requirement at the least (My computer has a i7 6700k, 16GB RAM and a GTX1080)

Enough about the cant's

This engine demonstrates Physically Based Rendering and several Shadow Mapping techniques
PBR is fed by data coming from custom made textures, where the R-channel is feeding roughness, the G-channel feeding Metallic and the B channeling feeding Ambient Occlusion

The Shadow mapping techniques are:

Regular Shadow Mapping (unfiltered) - done 

Percentage-Closer Filtering - done

Cascaded Shadow Mapping - W.I.P

Variance Shadow Mapping - Planned

Moment Shadow Mapping - Planned

I also use a shadow-atlas wherever I can (the same kind as DOOM uses as is seen here: http://www.adriancourreges.com/blog/2016/09/09/doom-2016-graphics-study/ )

![Screenshot](https://github.com/Themperror/ThempX11/blob/master/engine.jpg)
![Screenshot2](https://github.com/Themperror/ThempX11/blob/master/engine2.jpg)
