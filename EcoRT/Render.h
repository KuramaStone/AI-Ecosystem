#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include "glm/glm.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "BMath.h"

#include "ft2build.h"
#include FT_FREETYPE_H  

#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <random>
#include <map>

#include "Entity.h"
#include "VBOData.h"
#include "Render.h"
#include "BorderData.h"
#include "LifeTree.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_glfw.h" 

const int MAX_ENTITIES = 10000;
const int dataPerCell = 10;

struct TextData {
    b2Vec3 color;
    b2Vec2 position;
    int size;
    std::string string;
};

struct Character {
    unsigned int TextureID;  // ID handle of the glyph texture
    glm::ivec2   Size;       // Size of glyph
    glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
    unsigned int Advance;    // Offset to advance to next glyph
};

struct Circle {
    float x, y, radius;
    b2Vec3 color;
};

struct Line {
    float x1, y1, x2, y2, size;
    b2Vec3 color;
};

class BrainRender {
public:
    unsigned int brainShader;
    unsigned int textShader;
    unsigned int textVAO;
    unsigned int textVBO;
    std::map<char, Character>& characters;
    bool trimConnections = true;

    GLuint frameBuffer;
    GLuint renderedTexture;
    int uidOfLastEntity = -1;
    GLuint circleVao;
    GLuint circleVbo;
    GLuint lineVao;
    GLuint lineVbo;

    BrainRender(std::map<char, Character>& chars);
    void build();
    void render(int uid, CellBrain& brain, bool forceRender);
private:
    const int textureWidth = 512;
    const int textureHeight = 512;
    bool createdBuffer = false;
    int triangleAmountInCircle = 36;

    void drawBrain(CellBrain& brain, int textureWidth, int textureHeight);
    void drawAllCircles(std::vector<Circle>& circles);
    void drawAllLines(std::vector<Line>& lines);
    void renderText(std::vector<TextData>& textData);
};

struct BranchRenderData {
    float x;
    float y;
    float angle;
    float radius;
    std::vector<Circle>& circles;
    std::vector<Line>& lines;
    std::vector<TextData>& textData;
    float step;
    float px;
    float py;
    float thetaSpan;
    std::map<int, b2Vec3>& extantBranches;
};

class TreeOfLifeRenderer {
public:
    unsigned int brainShader;
    unsigned int textShader;

    unsigned int textVAO;
    unsigned int textVBO;
    std::map<char, Character>& characters;

    GLuint frameBuffer;
    GLuint renderedTexture;
    int uidOfLastEntity = -1;
    GLuint circleVao;
    GLuint circleVbo;
    GLuint lineVao;
    GLuint lineVbo;

    TreeOfLifeRenderer(std::map<char, Character>& chars);
    void build();
    void render(TreeOfLife& tree, std::map<int, b2Vec3>& extantBranches);
private:
    const int textureWidth = 1024;
    const int textureHeight = 1024;
    bool createdBuffer = false;
    int triangleAmountInCircle = 36;

    bool drawBranch(TreeBranch* branch, BranchRenderData& renderData);
    void drawTree(TreeOfLife& tree, std::map<int, b2Vec3>& extantBranches, int textureWidth, int textureHeight);
    void drawAllCircles(std::vector<Circle>& circles);
    void drawAllLines(std::vector<Line>& lines);
    void renderText(std::vector<TextData>& textData);
};

class Render {
private:
    b2Vec2 world_size;
    GLFWwindow* window = nullptr;

    VBOData screenVBO;
    VBOData cellDataVBO;
    GLuint cellDataVAO;
    float cellData[MAX_ENTITIES * dataPerCell];
    int numSegments = 20;
    unsigned int entityShader;
    unsigned int borderShader;
    unsigned int textShader;
    unsigned int noiseShader;
    unsigned int shadowShader;
    unsigned int shadowPerShader;
    VBOData dataVBO;
    GLuint borderVAO;
    VBOData borderVBO;
    glm::vec3 lightDirection; 
    glm::vec3 lightPosition;


    // Font rendering
    FT_Library ft;
    FT_Face face;
    std::map<char, Character> characters;
    unsigned int textTexture;
    GLuint textVAO;
    GLuint textVBO;

    std::vector<float> circleVertices;
    VBOData circleVerticesVBO;
    GLuint circleVerticesVAO;

    std::vector<float> entityVertices;
    VBOData entityVerticesVBO;
    GLuint entityVerticesVAO;

    GLuint caveVAO;
    GLuint caveVBO;
    GLuint caveEBO;
    int caveShader;
    unsigned int defShader;

    GLuint quadVAO;
    GLuint quadVBO;

    GLuint borderLineVao;
    GLuint borderLineVbo;
    std::vector<b2Vec3> borderLineColors;
    int borderLineCount;

    bool haveShadersLoaded = false;

    std::string LoadShaderSource(const std::string& filename);
    void renderEntities(float worldScale, double worldTime, glm::mat4& modelViewProjectionMatrix, std::vector<Entity*>& entities, std::vector<PlantEntity*>& plants, float offsetX, float offsetY);
    void renderBorder(double worldTime, glm::mat4& modelViewProjectionMatrix, Border& border, float seasonTime);
    void renderText(std::vector<TextData>& textData);
    void RenderImGui(glm::mat4 mvp, ImDrawData* draw_data);
    void buildBorder();
    void generateEntityVertices(int numVertices, float radius, float length);
    void initText();
    b2Vec3 mixColor(b2Vec3 a, b2Vec3 b, float perc);
    static std::string readShaderSource(const std::string& filePath);
    static unsigned int createShaderData(const std::string& shaderSource, GLenum shaderType);

public:
    // used for smoother translation and scaling
    b2Vec2 currentWorldViewCenter;
    b2Vec2 startWorldViewCenter;
    b2Vec2 nextWorldViewCenter;
    float currentViewTranslateVelocity = 0.0f;
    float viewTranslationSpeed = 0.0001f;
    float currentViewScale = 0.000001f;
    float startViewScale = 0.000001f;
    float nextViewScale = 1.0f;
    float viewScaleSpeed = 0.0001f;
    float currentViewScaleVelocity = 0.0f;
    float viewScaleRate = 1.0f;
    float viewTranslateRate = 1.0f;

    // used to set window position
    b2Vec2 nextWindowPosition = b2Vec2(50, 50);
    b2Vec2 nextWindowSize = b2Vec2(800, 800);

    GLuint depthMapTexture;
    GLuint shadowMapTexture;
    GLuint caveMeshVAO;
    GLuint caveMeshVBO;
    GLuint caveMeshEBO;
    GLuint depthMapFBO;
    GLuint shadowMapFBO; 
    glm::mat4 modelViewProjectionMatrix;

    float backgroundTexScale = 1.639;
    float backgroundTexSize = 0.041;
    float cameraHeight;
    float waterRenderHeight = 50.0f;

    BrainRender brainRenderer = BrainRender(characters);
    TreeOfLifeRenderer treeRenderer = TreeOfLifeRenderer(characters);
    b2Vec2 viewportSize;

    Render(b2Vec2 worldSize);
    void loadShaders();
    void init();
    void buildScreen();
    void buildGUI();
    void buildCavesModel(Border& border);
    void updateCaveModel(Border& border);
    void setCavesTexture(std::vector<float>& data, Border& border);
    void buildCavesTexture(int seed, Border& border);
    void initModelShadows(Border& border);
    void bakeModelShadows(Border& border);
    void updateViewCenter(float deltaTime, float offsetX, float offsetY, float scale);

    void render(float deltaTime, double worldTime, std::vector<Entity*>& entities, std::vector<PlantEntity*>& plants, Border& border, 
        float offsetX, float offsetY, float scale, EnvironmentBody* focusedEntity, std::vector<TextData>& textData, float seasonTime);
    int updateData(std::vector<Entity*>& entities, std::vector<PlantEntity*>& plants);
    GLFWwindow* getWindow();
    glm::mat4 calculateMVPMatrix(float offsetX, float offsetY, float scale, bool useAspectRatio);
    void renderNoiseOnFrameBuffer(GLuint& frameBuffer, GLuint& renderedTexture, int textureWidth, int textureHeight,
        b2Vec2 scale, float time, int layers, float exponent, float persistance, float threshold);

    void drawBorderLines(glm::mat4& modelViewProjectionMatrix);

    static TextData createTextData(std::string text, int x, int y, int size, b2Vec3 color);
    static unsigned int createShader(std::string vertex, std::string frag);
    static void checkError(int id);
    void cleanUp();
};