#### Supported file formats ####

__Importers__:

- 3D
- [3DS](https://en.wikipedia.org/wiki/.3ds)
- [3MF](https://en.wikipedia.org/wiki/3D_Manufacturing_Format)
- AC
- [AC3D](https://en.wikipedia.org/wiki/AC3D)
- ACC
- AMJ
- ASE (ASCII Scene Export)
- ASK
- B3D
- [BLEND](https://en.wikipedia.org/wiki/.blend_(file_format))
   - **SUPPORT DEPRECATED:** It is too time-consuming to maintain an undocumented, continuously evolving format which contains so much more than we need.
- [BVH](https://en.wikipedia.org/wiki/Biovision_Hierarchy)
- CSM (Character Studio Motion)
- COB (Caligari trueSpace 3D Object)
- [DAE/Collada](https://en.wikipedia.org/wiki/COLLADA)
- [DXF](https://en.wikipedia.org/wiki/AutoCAD_DXF)
- ENFF
- [FBX](https://en.wikipedia.org/wiki/FBX)
- [glTF 1.0](https://en.wikipedia.org/wiki/GlTF#glTF_1.0) + GLB
- [glTF 2.0](https://en.wikipedia.org/wiki/GlTF#glTF_2.0):
  At the moment for glTF2.0 the following extensions are supported:
  + KHR_lights_punctual ( 5.0 )
  + KHR_materials_pbrSpecularGlossiness ( 5.0 )
  + KHR_materials_unlit ( 5.0 )
  + KHR_texture_transform ( 5.1 under test )
- HMP (3D GameStudio Heightmap)
- IFC-STEP
- IQM (Inter-Quake Model)
- [IRR](https://irrlicht.sourceforge.io/?page_id=45) / [IRRMESH](https://www.irrlicht3d.org/index.php?t=592)
- [LWO](https://en.wikipedia.org/wiki/LightWave_3D)
- [LWS](https://www.renderhub.com/guides/3d-file-formats/lws)
- LXO (Foundry Modo)
   - **Note:** uses LWO loader
- [M3D](https://bztsrc.gitlab.io/model3d)
   - **REQUIRES OPT-IN:** enable cmake options `ASSIMP_BUILD_M3D_IMPORTER`
      - NOTE: author of `M3D` format abandoned assimp support; implementation is incomplete and buggy.
- MD2 (id Software, 1997)
- MD3 (id Software, 1999)
- MD5 (id Software, 2004)
- MDC (compressed MD3)
- MDL (id Software, 1996)
- MESH / MESH.XML
- MOT
- MS3D
- NDO (Izware Nendo)
- [NFF](https://paulbourke.net/dataformats/nff/nff1.html)
- [OBJ](https://en.wikipedia.org/wiki/Wavefront_.obj_file)
- [OFF](https://en.wikipedia.org/wiki/OFF_(file_format))
- [OGEX](https://en.wikipedia.org/wiki/Open_Game_Engine_Exchange)
- [PLY](https://en.wikipedia.org/wiki/PLY_(file_format))
- PMX (MikuMikuDance)
- PRJ (3D Studio project file)
- Q3O (Quick3D object)
- Q3S (Quick3D scene)
- RAW
- SCN (Caligari trueSpace)
   - **Note:** uses COB loader 
- SIB (Nevercenter Silo Binary)
- SMD (StudioModel Data)
- [STEP/STP](https://en.wikipedia.org/wiki/ISO_10303-21)
- [STL](https://en.wikipedia.org/wiki/STL_(file_format))
- TER (Terragen Terrain)
- UC
- [USD](https://en.wikipedia.org/wiki/Universal_Scene_Description)
   - **REQUIRES OPT-IN:** enable cmake option `ASSIMP_BUILD_USD_IMPORTER`
   - To use latest v1.0.0 USD release also enable cmake option `ASSIMP_USD_BETA`
- VRML/WRL
   - **REQUIRES OPT-IN:** enable cmake option `ASSIMP_BUILD_VRML_IMPORTER`
- VTA (evolution of SMD)
- X (DirectX)
- [X3D](https://en.wikipedia.org/wiki/X3D)
- [XGL](https://www.okino.com/convert/_what_is/what_is_xgl.htm)
- [ZGL](https://www.okino.com/convert/_what_is/what_is_xgl.htm)

Additionally, some formats are supported by dependency on non-free code or external SDKs (not built by default):

- [C4D](https://en.wikipedia.org/wiki/Cinema_4D) (https://github.com/assimp/assimp/wiki/Cinema4D-&-Melange) IMporting geometry + node hierarchy are currently supported

__Exporters__:

- [3DS](https://en.wikipedia.org/wiki/.3ds)
- [3MF](https://en.wikipedia.org/wiki/3D_Manufacturing_Format) ( experimental )
- ASSBIN
- ASSJSON (for WebGl, via https://github.com/acgessler/assimp2json)
- ASSXML
- [DAE/Collada](https://en.wikipedia.org/wiki/COLLADA)
- [FBX](https://en.wikipedia.org/wiki/FBX) ( experimental )
- [glTF 1.0](https://en.wikipedia.org/wiki/GlTF#glTF_1.0) (partial)
- [glTF 2.0](https://en.wikipedia.org/wiki/GlTF#glTF_2.0) (partial)
- [M3D](https://bztsrc.gitlab.io/model3d)
    - **REQUIRES OPT-IN:** enable cmake option `ASSIMP_BUILD_M3D_EXPORTER`
        - NOTE: author of `M3D` format abandoned assimp support; implementation is incomplete and buggy.
- [OBJ](https://en.wikipedia.org/wiki/Wavefront_.obj_file)
- [OGEX](https://en.wikipedia.org/wiki/Open_Game_Engine_Exchange)
- [PLY](https://en.wikipedia.org/wiki/PLY_(file_format))
- [PBRTv4](https://github.com/mmp/pbrt-v4)
- [STEP/STP](https://en.wikipedia.org/wiki/ISO_10303-21)
- [STL](https://en.wikipedia.org/wiki/STL_(file_format))
- X (DirectX)
