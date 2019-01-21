//
//  Copyright 2018 Alun Evans. All rights reserved.
//
#include "GraphicsSystem.h"
#include "Parsers.h"
#include "extern.h"
#include <algorithm>

//destructor
GraphicsSystem::~GraphicsSystem() {
	//delete shader pointers
	for (auto shader_pair : shaders_) {
		if (shader_pair.second)
			delete shader_pair.second;
	}
}

//set initial state of graphics system
void GraphicsSystem::init(int window_width, int window_height) {
    //set 'background' colour of framebuffer
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glViewport(0, 0, window_width, window_height);
    //enable culling and depth test
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE); //enable culling
    glCullFace(GL_BACK); //which face to cull
}

//called after loading everything
void GraphicsSystem::lateInit() {

	//sort meshes by shader and material
    sortMeshes_();
}

void GraphicsSystem::updateMainViewport(int window_width, int window_height) {
	glViewport(0, 0, window_width, window_height);
}


//This function executes two sorts:
// i) sorts materials array by shader_id
// ii) sorts Mesh components by material id
//the result is that the mesh component array is
//ordered by both shader and material
void GraphicsSystem::sortMeshes_(){
   
	/* 1st: SORT ALL MATERIALS BY SHADER ID */

    //sort materials by shader id
    //first we store the old index of each material in materials_ array
	//use materials_[i].index
	//...
	    
    //second, use std::sort to sort materials by shader_id
	//you will need to use a lambda funcion
	//...
        
    //now we map old indices to new indices
    std::map<int, int> old_new;
    //...

    //now, for all meshes we swap index of materials to new index
    auto& meshes = ECS.getAllComponents<Mesh>();
    for (auto& mesh : meshes) {
		//..
    }
    
	/* 1st: SORT ALL MESHES BY MATERIAL ID */

    //same as above, store old mesh indices in index property
	//...

	//use std::sort to sort meshes by material id
    //...
    
    //clear map and refill with mesh index map
    old_new.clear();
    //...
    
    //update all entities with new mesh id
	//only need to uncomment this
    /*auto& all_entities = ECS.entities;
    for (auto& ent : ECS.entities) {
        int old_index = ent.components[type2int<Mesh>::result];
        int new_index = old_new[old_index];
        ent.components[type2int<Mesh>::result] = new_index;
    }*/
}

void GraphicsSystem::checkShaderAndMaterial(Mesh& mesh) {
    //get shader id from material. if same, don't change
    if (!shader_ || shader_->program != materials_[mesh.material].shader_id) {
		useShader(materials_[mesh.material].shader_id);
    }
    //set material uniforms if required
    if (current_material_ != mesh.material) {
        current_material_ = mesh.material;
        setMaterialUniforms();
    }
}

void GraphicsSystem::update(float dt) {
    
    //set initial OpenGL state
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    //reset shader and material
    useShader((GLuint)0);
    current_material_ = -1;
    
	//update cameras
	auto& cameras = ECS.getAllComponents<Camera>();
	for (auto &cam : cameras) cam.update();

	auto& mesh_components = ECS.getAllComponents<Mesh>();
	for (auto &mesh : mesh_components) {
        checkShaderAndMaterial(mesh);
		renderMeshComponent_(mesh);
	}
    
}

//sets uniforms for current material and current shader
void GraphicsSystem::setMaterialUniforms() {
    Material& mat = materials_[current_material_];
    
	//TODO: 
	// - Replace glGetUniformLocation calls with shader_->setUniform
	// - Don't do it for lights though. Why not?


    //material uniforms
    GLint u_ambient = glGetUniformLocation(shader_->program, "u_ambient");
    if (u_ambient != -1) glUniform3fv(u_ambient, 1, mat.ambient.value_);

    GLint u_diffuse = glGetUniformLocation(shader_->program, "u_diffuse");
    if (u_diffuse != -1) glUniform3fv(u_diffuse, 1, mat.diffuse.value_);

    GLint u_specular = glGetUniformLocation(shader_->program, "u_specular");
    if (u_specular != -1) glUniform3fv(u_specular, 1, mat.specular.value_);
    
    GLint u_specular_gloss = glGetUniformLocation(shader_->program, "u_specular_gloss");
    if (u_specular_gloss != -1) glUniform1f(u_specular_gloss, 200.0f); //...1f - for float
    
    //texture uniforms
    GLint u_diffuse_map = glGetUniformLocation(shader_->program, "u_diffuse_map");
    if (u_diffuse_map != -1) glUniform1i(u_diffuse_map, 0); // ...1i - is integer
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mat.diffuse_map);


    
    //light uniforms
    const std::vector<Light>& lights = ECS.getAllComponents<Light>();  // get number of lights in scene from ECM
    
    GLint u_num_lights = glGetUniformLocation(shader_->program, "u_num_lights"); //get/set uniform in shader
    if (u_num_lights != -1) glUniform1i(u_num_lights, (int)lights.size());
    
    //for each light
    for (size_t i = 0; i < lights.size(); i++) {
        //get light transform
        Transform& light_transform = ECS.getComponentFromEntity<Transform>(lights[i].owner);
        
        //position
        std::string light_position_name = "lights[" + std::to_string(i) + "].position"; // create dynamic uniform name
        GLint u_light_pos = glGetUniformLocation(shader_->program, light_position_name.c_str()); //find it
        if (u_light_pos != -1) glUniform3fv(u_light_pos, 1, light_transform.position().value_); // light position
        
        //color
        std::string light_color_name = "lights[" + std::to_string(i) + "].color";
        GLint u_light_col = glGetUniformLocation(shader_->program, light_color_name.c_str());
        if (u_light_col != -1) glUniform3fv(u_light_col, 1, lights[i].color.value_);
    }
}

//renders a given mesh component
void GraphicsSystem::renderMeshComponent_(Mesh& comp) {
    
    //get transform of components entity
    Transform& transform = ECS.getComponentFromEntity<Transform>(comp.owner);
	//get camera
	Camera& cam = ECS.getComponentInArray<Camera>(ECS.main_camera);
    //get Geometry, material and textures
    Geometry& geom = geometries_[comp.geometry];
   
	//model matrix
	lm::mat4 model_matrix = transform.getGlobalMatrix(ECS.getAllComponents<Transform>());
	//Model view projection matrix
	lm::mat4 mvp_matrix = cam.view_projection * model_matrix;

	//view frustum culling
    if (!BBInFrustum_(geom.aabb, mvp_matrix)) {
        return;
    }
	
	//std::cout << ECS.entities[comp.owner].name << "-";

	//normal matrix
	lm::mat4 normal_matrix = model_matrix;
	normal_matrix.inverse();
	normal_matrix.transpose();
    
	//TODO: update uniforms using shader_->setUniform

    //transform uniforms
    GLint u_mvp = glGetUniformLocation(shader_->program, "u_mvp");
    if (u_mvp != -1) glUniformMatrix4fv(u_mvp, 1, GL_FALSE, mvp_matrix.m);

    
    GLint u_model = glGetUniformLocation(shader_->program, "u_model");
    if (u_model != -1) glUniformMatrix4fv(u_model, 1, GL_FALSE, model_matrix.m);


	GLint u_normal_matrix = glGetUniformLocation(shader_->program, "u_normal_matrix");
	if (u_normal_matrix != -1) glUniformMatrix4fv(u_normal_matrix, 1, GL_FALSE, normal_matrix.m);


    GLint u_cam_pos = glGetUniformLocation(shader_->program, "u_cam_pos");
    if (u_cam_pos != -1) glUniform3fv(u_cam_pos, 1, cam.position.value_); // ...3fv - is array of 3 floats

    
    //tell OpenGL we want to the the vao_ container with our buffers
    glBindVertexArray(geom.vao);
    //draw our geometry
    glDrawElements(GL_TRIANGLES, geom.num_tris * 3, GL_UNSIGNED_INT, 0);
    //tell OpenGL we don't want to use our container anymore
    glBindVertexArray(0);
    
}
//
////********************************************
//// Adding and creating functions
////********************************************

//change shader only if need to - note shader object must be in shaders_ map
//s - pointer to a shader object
void GraphicsSystem::useShader(Shader* s) {
    if (!s) {
        glUseProgram(0);
        shader_ = nullptr;
    }
    else if (!shader_ || shader_ != s){
        glUseProgram(s->program);
        shader_ = s;
    }
}

//change shader only if need to - note shader object must be in shaders_ map
//p - GL id of shader
void GraphicsSystem::useShader(GLuint p) {
    if (!p) {
        glUseProgram(0);
        shader_ = nullptr;
    }
    else if (!shader_ || shader_->program != p) {
        glUseProgram(p);
        shader_ = shaders_[p];
    }
}

//loads a shader, stores it in a map where key is it's program id, and returns a pointer to shader
//-vs: either the path to the vertex shader, or the vertex shader string
//-fs: either the path to the fragment shader, or the fragment shader string
//-compile_direct: if false, assume other two parameters are paths, if true, assume they are shader strings
Shader* GraphicsSystem::loadShader(std::string vs, std::string fs, bool compile_direct) {
	Shader* new_shader;
	if (compile_direct) {
		new_shader = new Shader();
		new_shader->compileFromStrings(vs, fs);
	}
	else {
		new_shader = new Shader(vs, fs);
	}
	shaders_[new_shader->program] = new_shader;
	return new_shader;
}

//create a new material and return pointer to it
int GraphicsSystem::createMaterial() {
    materials_.emplace_back();
    return (int)materials_.size() - 1;
}

//creates a standard plane geometry and return its
int GraphicsSystem::createPlaneGeometry(){
    
    std::vector<GLfloat> vertices, uvs, normals;
    std::vector<GLuint> indices;
    vertices = { -0.5f, -0.5f, 0.0f,    0.5f, -0.5f, 0.0f, 0.5f,  0.5f, 0.0f, -0.5f, 0.5f, 0.0f };
    uvs = { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };
    normals = { 0.0f, 0.0f, 1.0f,    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,    0.0f, 0.0f, 1.0f };
    indices = { 0, 1, 2, 0, 2, 3 };
    //generate the OpenGL buffers and create geometry
    GLuint vao = generateBuffers_(vertices, uvs, normals, indices);
    geometries_.emplace_back(vao, 2);
    
    return (int)geometries_.size() - 1;
}

//create geometry from
//returns index in geometry array with stored geometry data
int GraphicsSystem::createGeometryFromFile(std::string filename) {
    
    std::vector<GLfloat> vertices, uvs, normals;
    std::vector<GLuint> indices;
    //check for supported format
    std::string ext = filename.substr(filename.size() - 4, 4);
    if (ext == ".obj" || ext == ".OBJ")
    {
        //fill it with data from object
        if (Parsers::parseOBJ(filename, vertices, uvs, normals, indices)) {
            
            //generate the OpenGL buffers and create geometry
            GLuint vao = generateBuffers_(vertices, uvs, normals, indices);
            geometries_.emplace_back(vao, (GLuint)indices.size() / 3);

			//create AABB
			setGeometryAABB_(geometries_.back(), vertices);

            return (int)geometries_.size() - 1;
        }
        else {
            std::cerr << "ERROR: Could not parse mesh file" << std::endl;
            return -1;
        }
    }
    else {
        std::cerr << "ERROR: Unsupported mesh format when creating geometry" << std::endl;
        return -1;
    }
    
}

// Given an array of floats (in sets of three, representing vertices) calculates and
// sets the AABB of a geometry
void GraphicsSystem::setGeometryAABB_(Geometry& geom, std::vector<GLfloat>& vertices) {

	//TODO
	//- iterate vertices array
	//- set geom.aabb center and halfwidth in each axis

}

//tests whether Bounding box is inside frustum or not, based on model_view_projection matrix
bool GraphicsSystem::BBInFrustum_(const AABB& aabb, const lm::mat4& mvp) {
    //each corner point of box gets transformed into clip space, to give point PC, in HOMOGENOUS coords
    //point is inside clip space iff
    //-PC.w < PC.xyz < PC.w
    //so we first take each corner of AABB (note, using vec4 because of homogenous coords) and multiply
    //by matrix to clip space. Then we test each point against the 6 planes e.g. PC is on the 'right' side
    //of the left plane iff -PC.w < PC.x; and is on 'left' side of right plane is PC.x < PC.w etc.
    //For more info see:
    //http://www.lighthouse3d.com/tutorials/view-frustum-culling/clip-space-approach-extracting-the-planes/
    
    
    //the eight points of the box corners are calculated using center and +/- halfwith:
    //- - -
    //- - +
    //- + -
    //- + +
    //+ - -
    //+ - +
    //+ + -
    //+ + +
	lm::vec4 points[8];
	points[0] = lm::vec4(aabb.center.x - aabb.half_width.x, aabb.center.y - aabb.half_width.y, aabb.center.z - aabb.half_width.z, 1.0);
	points[1] = lm::vec4(aabb.center.x - aabb.half_width.x, aabb.center.y - aabb.half_width.y, aabb.center.z + aabb.half_width.z, 1.0);
	points[2] = lm::vec4(aabb.center.x - aabb.half_width.x, aabb.center.y + aabb.half_width.y, aabb.center.z - aabb.half_width.z, 1.0);
	points[3] = lm::vec4(aabb.center.x - aabb.half_width.x, aabb.center.y + aabb.half_width.y, aabb.center.z + aabb.half_width.z, 1.0);
	points[4] = lm::vec4(aabb.center.x + aabb.half_width.x, aabb.center.y - aabb.half_width.y, aabb.center.z - aabb.half_width.z, 1.0);
	points[5] = lm::vec4(aabb.center.x + aabb.half_width.x, aabb.center.y - aabb.half_width.y, aabb.center.z + aabb.half_width.z, 1.0);
	points[6] = lm::vec4(aabb.center.x + aabb.half_width.x, aabb.center.y + aabb.half_width.y, aabb.center.z - aabb.half_width.z, 1.0);
	points[7] = lm::vec4(aabb.center.x + aabb.half_width.x, aabb.center.y + aabb.half_width.y, aabb.center.z + aabb.half_width.z, 1.0);

	//transform to clip space
	lm::vec4 clip_points[8];
	for (int i = 0; i < 8; i++) {
		clip_points[i] = mvp * points[i];
	}

	//left plane
	int in = 0;
	for (int i = 0; i < 8; i++) {
		if (-clip_points[i].w < clip_points[i].x) in++;
	}
	if (!in) return false;

	//right plane
	in = 0;
	for (int i = 0; i < 8; i++) {
		if (clip_points[i].x < clip_points[i].w) in++;
	}
	if (!in) return false;

	//bottom plane
	in = 0;
	for (int i = 0; i < 8; i++) {
		if (-clip_points[i].w < clip_points[i].y) in++;
	}
	if (!in) return false;

	//top plane
	in = 0;
	for (int i = 0; i < 8; i++) {
		if (clip_points[i].y < clip_points[i].w) in++;
	}
	if (!in) return false;

	//near plane
	in = 0;
	for (int i = 0; i < 8; i++) {
		if (-clip_points[i].z < clip_points[i].z) in++;
	}
	if (!in) return false;

	//far plane
	in = 0;
	for (int i = 0; i < 8; i++) {
		if (clip_points[i].z < clip_points[i].w) in++;
	}
	if (!in) return false;

	return true;

}

//generates buffers in VRAM and returns VAO handle.
GLuint GraphicsSystem::generateBuffers_(std::vector<float>& vertices, std::vector<float>& uvs, std::vector<float>& normals, std::vector<unsigned int>& indices) {
    //generate and bind vao
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    GLuint vbo;
    //positions
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &(vertices[0]), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    //texture coords
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(float), &(uvs[0]), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    //normals
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), &(normals[0]), GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
    //indices
    GLuint ibo;
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &(indices[0]), GL_STATIC_DRAW);
    //unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return vao;
}


