This project implements environment mapping and sky shading in OpenGL. It includes rendering a skybox, sampling environment maps, and applying reflective materials using cube maps. The project demonstrates how direction vectors from the camera are used to fetch texels from an environment texture to produce realistic lighting and reflections.

**Features**
- Skybox rendering using cube-map textures  
- Environment map sampling for reflective surfaces  
- Real-time shading based on view direction  
- Configurable camera and scene setup  
- Simple scene demonstrating reflective materials

**How to Run**
1. Clone the repository  
2. Build using CMake  
3. Run the executable from your build directory  
4. Move the camera to see reflections update in real time

**Project Structure**
- src/: core rendering and application logic  
- shaders/: vertex + fragment shaders for skybox and environment shading  
- includes/: utilities (shader loader, camera controls)  
- resources/: skybox textures and sample cube maps

**Dependencies**
- OpenGL 3.3+  
- GLFW  
- GLAD  
- glm math library  
- CMake

**Controls**
- WASD → move camera  
- Mouse → rotate camera view  
- R → reset camera  
- F → toggle reflection mode (if implemented)

**Future Improvements**
- Image-based lighting (IBL) with irradiance and prefiltered maps  
- Roughness-based reflections (GGX)  
- Procedural sky generation  
- HDR support and tone mapping 
