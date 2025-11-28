/**
 * @file main.cpp
 * @brief Main application file for OpenGL with ImGui Template.
 *
 * This file contains the main function and initialization routines for an
 * OpenGL application using ImGui for rendering the GUI components.
 *
 * @author Mustafa Alotbah
 * @date 08 Dec 2023
 * @version 0.1
 * @terms_of_use 'Creative Commons Attribution 4.0 International (CC BY 4.0)'
 */

#include "utility/Layer2D.h"
#include <cmath>
#include <string>
#include <format>
#include <utility>

// M_PI is not defined in MSVC by default
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


// Uniform variable names used in shaders.
std::string UNIFORM_COLOR = "iColor";
std::string UNIFORM_HIGHLIGHT = "highlightIntensity";


// Vertex shader code for basic object rendering.
std::string vertexShader = R"(
    #version 330 core
    layout(location=0) in vec4 position;

    uniform mat3 camera;
    uniform mat3 transform;

    out vec2 fragCoord;
    out vec2 ndcCoord;

    void main() {
       fragCoord = position.xy;
       vec3 tr_position = camera * transform * vec3(position.xy, 1.0);
       ndcCoord = tr_position.xy;
       gl_Position = vec4(tr_position.xy, 0.0, tr_position.z);
    }
)";

// Fragment shader code for color and highlighting effects.
std::string fragmentShader =R"(
    #version 330 core
    layout(location=0) out vec4 fragColor;

    in vec2 fragCoord;
    in vec2 ndcCoord;

    uniform vec4 iColor;
    uniform vec2 mousePos; // Mouse position
    uniform float highlightIntensity;    // Uniform to control highlighting

    void main() {
        float distanceToMouse = distance(ndcCoord, mousePos);
        float highlightIntensity_ = 1.0 - smoothstep(0.0, 0.25, distanceToMouse);
        highlightIntensity_ *= highlightIntensity;

        // Slightly lighter color for highlight
        vec4 lighterColor = iColor + 0.5;
        lighterColor = clamp(lighterColor, 0.0, 1.0);

        fragColor = mix(iColor, lighterColor, highlightIntensity_); // Adjust the intensity
    }
)";

// Global shader initialization.
std::shared_ptr<Shader> Shaders::flatShader = nullptr;

//region -------------------------------- Material ----------------------------

// Material class implementation.
// Constructor: Initializes a material with a shader and a base color.
Material::Material(std::shared_ptr<Shader> shader, const glm::vec4& color)
        : shader(std::move(shader)), color(color) {}

// Bind the material to the current context.
void Material::bind() const {
    shader->bind();
    shader->setUniform4f(UNIFORM_COLOR, color.r, color.g, color.b, color.a);
    shader->setUniform1f(UNIFORM_HIGHLIGHT, highlightIntensity);
}

// Initialize predefined colors for convenience.
std::shared_ptr<Material> Material::Black = nullptr;
std::shared_ptr<Material> Material::White = nullptr;
std::shared_ptr<Material> Material::DarkGrey = nullptr;
std::shared_ptr<Material> Material::Grey = nullptr;
std::shared_ptr<Material> Material::Red = nullptr;
std::shared_ptr<Material> Material::Green = nullptr;
std::shared_ptr<Material> Material::Blue = nullptr;
std::shared_ptr<Material> Material::Yellow = nullptr;

// Create a flat material with a specific color.
std::shared_ptr<Material> Material::createFlatMaterial(const glm::vec4& color) {
    return std::make_shared<Material>(Shaders::flatShader, color);
}

// Initialize global materials with predefined colors.
void Material::InitializeGlobalMaterials() {
    Black = createFlatMaterial(glm::vec4(0, 0, 0, 1));
    White = createFlatMaterial(glm::vec4(1, 1, 1, 1));
    DarkGrey = createFlatMaterial(glm::vec4(0.25, 0.25, 0.25, 1));
    Grey = createFlatMaterial(glm::vec4(0.5, 0.5, 0.5, 1));
    Red = createFlatMaterial(glm::vec4(1.0, 0.0, 0.0, 1));
    Green = createFlatMaterial(glm::vec4(0.0, 1.0, 0.0, 1));
    Blue = createFlatMaterial(glm::vec4(0.0, 0.0, 1.0, 1.0));
    Yellow = createFlatMaterial(glm::vec4(1.0, 1.0, 0.01, 1.0));
}

void Material::setAlpha(float alpha) {
    color.a = alpha;
}

void Material::setHighlight(float highlight) {
    highlightIntensity = highlight;
}

glm::vec4 Material::getColorViridis(float value, float alpha) {
    if (value <= 0.0) value = 0.0;
    if (value > 1.0) value = 1.0;
    unsigned char* rgb = ColormapTables::viridis_table[(size_t)(value * 511)];
    glm::vec4 res = {float(rgb[0])/255.0f, float(rgb[1])/255.0f, float(rgb[2])/255.0f, alpha};
    return res;
}

//endregion


//region ------------------------------- KiwiFrame ----------------------------
KiwiFrame::KiwiFrame() {
    // set up the buffers
    {
        GL_TRY(glGenFramebuffers(1, &frameBufferObject));

        // Generate one texture and store its ID in 'textureId' and bind it to 2D texture.
        GL_TRY(glGenTextures(1, &textureId));
        GL_TRY(glBindTexture(GL_TEXTURE_2D, textureId));

        // set the dimensions right (and keep updating them later)
        GL_TRY(glViewport(0, 0, frameSize.x, frameSize.y));

        // Define and allocate memory for the texture parameters.
        GL_TRY(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (int) frameSize.x, (int) frameSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE,
                            nullptr));
        GL_TRY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));   // Set minifying filter to linear.
        GL_TRY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));   // Set magnifying filter to linear.

        // Create a Renderbuffer Object for Depth Testing
        GL_TRY(glGenRenderbuffers(1, &renderBufferObject));
        GL_TRY(glBindRenderbuffer(GL_RENDERBUFFER, renderBufferObject));
        GL_TRY(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (int) frameSize.x, (int) frameSize.y));
        GL_TRY(glBindRenderbuffer(GL_RENDERBUFFER, 0));

        // Attach the Renderbuffer Object to the Framebuffer
        GL_TRY(glBindFramebuffer(GL_FRAMEBUFFER, frameBufferObject));
        GL_TRY(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderBufferObject));

        // Check if framebuffer is complete
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
        GL_TRY(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    }
}
KiwiFrame::~KiwiFrame() {
    // Delete the texture
    if (textureId != 0) glDeleteTextures(1, &textureId);

    // Delete the framebuffer object
    if (frameBufferObject != 0) glDeleteFramebuffers(1, &frameBufferObject);

    // Delete the render buffer object
    if (renderBufferObject != 0) glDeleteRenderbuffers(1, &renderBufferObject);
}

void KiwiFrame::resize(int width, int height) {
    if (width == 0 || height == 0) return; // Handle minimization case

    frameSize = {width, height};

    // Reallocate texture
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    // Reallocate renderbuffer storage
    glBindRenderbuffer(GL_RENDERBUFFER, renderBufferObject);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    // Update viewport
    glViewport(0, 0, width, height);
}
void KiwiFrame::bind() const {
    // Set up the framebuffer and texture for drawing.
    // Attach the texture to the framebuffer.
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferObject);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);

    // update the dimension, so the aspect is correct when the user changes the window size
    glViewport(0, 0, (int) frameSize.x, (int) frameSize.y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void KiwiFrame::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void KiwiFrame::saveFrameAsImage(const std::string &filename) {
    // Bind the framebuffer and set the correct viewport
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferObject);
    glViewport(0, 0, frameSize.x, frameSize.y);

    // Read pixels
    std::vector<unsigned char> pixels(frameSize.x * frameSize.y * 3); // 3 for RGB
    glReadPixels(0, 0, frameSize.x, frameSize.y, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Unbind the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Flip the image vertically as OpenGL's origin is bottom-left
    int width = frameSize.x;
    int height = frameSize.y;
    int rowSize = width * 3;
    std::vector<unsigned char> flippedPixels(rowSize * height);
    for (int y = 0; y < height; y++) {
        memcpy(flippedPixels.data() + (height - y - 1) * rowSize, pixels.data() + y * rowSize, rowSize);
    }

    // Save to file
    stbi_write_jpg(filename.c_str(), width, height, 3, flippedPixels.data(), rowSize);
}

//endregion


//region -------------------------------- KiwiCore ----------------------------

KiwiCore::KiwiCore() {
    // initialize OpenGL globals
    Shaders::flatShader = std::make_shared<Shader>(vertexShader, fragmentShader);
}

void KiwiCore::addLayer(std::shared_ptr<KiwiLayer> layer) {
    layers.push_back(std::move(layer));
}



void KiwiCore::renderFrame(float width, float height, double time, double deltaTime) {
    // Check if the size has changed
    if (frame.size().x != width || frame.size().y != height) {    // TODO better checking
        frame.resize((int)width, (int)height);
        for (const std::shared_ptr<KiwiLayer>& layer: layers) {
            layer->getCamera().setAspectRatio(width, height);
        }
    }
    frame.bind();
    for (const std::shared_ptr<KiwiLayer>& layer: layers) {
        layer->render(width, height, time, deltaTime);
    }
    frame.unbind();

}

void KiwiCore::pollEvents(glm::vec2 windowPos, glm::vec2 mousePos, InputState inputState) {
    // update origins
    calcNormalizedMousePos(windowPos, mousePos);


    // TODO add other buttons
    MouseEvent::Type mouseEventType = MouseEvent::Type::Enter;
    bool resetButton = false;

    if (inputState.mouseButton != Event::MouseButton::None &&
        state.input.mouseButton != inputState.mouseButton) {
        state.input.mouseButton = inputState.mouseButton;
        mouseEventType = MouseEvent::Type::Click;
    }
    else if (inputState.mouseButton != Event::MouseButton::None) {
        mouseEventType = MouseEvent::Type::Down;
    }
    else if (inputState.mouseButton == Event::MouseButton::None &&
             state.input.mouseButton != Event::MouseButton::None) {
        state.input.mouseButton = Event::MouseButton::None;
        mouseEventType = MouseEvent::Type::Release;
    }

    for (auto & layer : layers) {

        layer->updateMousePosition(state.normalizedMousePos);

        if (abs(inputState.deltaMouseWheel) > 0.01f) {
            auto e = MouseEvent(state.normalizedMousePos, MouseEvent::Type::MouseWheel);
            e.deltaWheel = inputState.deltaMouseWheel;
            layer->handleMouseEvent(e);
        }

        auto e = MouseEvent(state.normalizedMousePos, mouseEventType, inputState.mouseButton);
        layer->handleMouseEvent(e);

    }



}

void KiwiCore::calcNormalizedMousePos(glm::vec2 windowPos, glm::vec2 mousePos){
    // TODO this is common all layers
    glm::vec2 frameTopLeftPos = glm::vec2(windowPos.x, windowPos.y);

    state.origin = glm::vec2(
            frameTopLeftPos.x + frameSize().x / 2,
            frameTopLeftPos.y + frameSize().y / 2);

    state.normalizedMousePos = glm::vec2(
            2*(mousePos.x - state.origin.x)/frameSize().x,
            2*(-mousePos.y + state.origin.y)/frameSize().y
    );
}




//endregion


//region ------------------------------ KiwiLayer2D ---------------------------

void KiwiLayer2D::render(float windowWidth, float windowHeight, double time, double deltaTime) {
    Shaders::flatShader->bind();
    Shaders::flatShader->setUniform3x3f("camera", camera.getTransformation());
    Shaders::flatShader->setUniform2f("mousePos", mouseNormalizedPosition.x, mouseNormalizedPosition.y);

    grid_.draw();

    for (const std::shared_ptr<Object2D>& obj: drawList) {
        Shaders::flatShader->setUniform3x3f("transform", obj->transform);
        obj->draw();
    }
}

void KiwiLayer2D::registerComponent(std::shared_ptr<KiwiComponent2D> component) {
    components.push_back(std::move(component));
}

KiwiLayer2D::KiwiLayer2D() : grid_(100, 1, 5) {
    Material::InitializeGlobalMaterials();
    Shaders::flatShader->setUniform3x3f("camera", glm::mat3(1.0));
    Shaders::flatShader->setUniform1f("highlightIntensity", 0.9f);
    grid_.update(camera.getZoom(), camera.getPosition());

}

void KiwiLayer2D::updateMousePosition(glm::vec2 normalizedPosition) {
    mouseNormalizedPosition = normalizedPosition;
    glm::mat3 inverseCameraTransform = camera.getInverseTransformation();
    mousePosition = inverseCameraTransform * glm::vec3(normalizedPosition, 1.0f);
}

void KiwiLayer2D::handleMouseEvent(MouseEvent mouseEvent) {

    if (mouseEvent.type == MouseEvent::Type::MouseWheel) {

        float currentZoom = getCamera().getZoom();
        glm::vec2 currentCamPos = getCamera().getPosition();

        // Set the new zoom
        float log_zoom = std::log(currentZoom * 3906.25f) / std::log(5.0f);     // get exponential space
        float cam_zoom = std::max(0.00001f, log_zoom + mouseEvent.deltaWheel/50);    // modify exponential space
        float newZoom = 0.000256f * std::pow(5.0f, cam_zoom);                   // reset in linear space
        getCamera().setZoom(newZoom);
        updateMousePosition(mouseNormalizedPosition);

        // Update the grid with the new zoom and camera position
        getGrid().update(newZoom, camera.getPosition());
        return;
    }

    if (mouseEvent.button == Event::MouseButton::Middle) {
        if (mouseEvent.type == MouseEvent::Type::Click) {
            prevMousePosition = mouseNormalizedPosition / camera.getZoom();
            prevCameraPosition = camera.getPosition();
        }

        if (mouseEvent.type == MouseEvent::Type::Down) {
            camera.setPosition(prevCameraPosition + mouseNormalizedPosition / camera.getZoom() - prevMousePosition);
        }
        return;
    }

    if (mouseEvent.type == MouseEvent::Type::Click) {
        if(onMouseClickCallback && !onMouseClickCallback(mouseEvent, *this)) return;
    }
    else if (mouseEvent.type == MouseEvent::Type::Down) {
        if(onMouseDownCallback && !onMouseDownCallback(mouseEvent, *this)) return;
    }
    else if (mouseEvent.type == MouseEvent::Type::Release) {
        if(onMouseReleaseCallback && !onMouseReleaseCallback(mouseEvent, *this)) return;
    }

    for (const std::shared_ptr<KiwiComponent2D>& comp: components) {
        glm::vec4 boundingBox = comp->getBoundingBox();
        if (mousePosition.x > boundingBox.x && mousePosition.y > boundingBox.y &&
            mousePosition.x < boundingBox.z && mousePosition.y < boundingBox.a) {

            // Mouse Enter
            if (!comp->isHovering()) {
                comp->onMouseEvent(MouseEvent(mousePosition, MouseEvent::Type::Enter));
            }

            // Mouse First Click Down (has to be on object)
            if (mouseEvent.type == MouseEvent::Type::Click) {
                comp->onMouseEvent(MouseEvent(mousePosition, mouseEvent.type));
            }

        }

        // Mouse left
        else if(comp->isHovering()) {
            comp->onMouseEvent(MouseEvent(mousePosition, MouseEvent::Type::Leave));
        }

        // Release previously "downed" components (no matter if hovering or not)
        if ((mouseEvent.type == MouseEvent::Type::Release ||
                mouseEvent.type == MouseEvent::Type::Down ) && comp->isMouseDown()) {
            comp->onMouseEvent(MouseEvent(mousePosition, mouseEvent.type));
        }

    }
}

//endregion


//region ------------------------------- NestGrid2D ---------------------------
NestGrid2D::NestGrid2D(int size, float spacing, float divisionBy) {

    spacingMajor = spacing;
    division = divisionBy;

    grid2D = std::make_shared<Grid2D>(Material::createFlatMaterial(
            glm::vec4(0.3, 0.3, 0.3, 1)), size, spacing);
    grid2D->getStrokeMaterial()->setHighlight(0.05);

    subGrid2D = std::make_shared<Grid2D>(Material::createFlatMaterial(
            glm::vec4(0.3, 0.3, 0.3, 1)), size, spacingMajor / division);
    subGrid2D->getStrokeMaterial()->setHighlight(0.05);

    grid2D->baseSpacing = spacingMajor;
    subGrid2D->baseSpacing = spacingMajor / division;
    subGrid2D->flipAlpha = true;
}

void NestGrid2D::update(float zoomLevel, glm::vec2 cameraPosition) {
    grid2D->updateGrid(zoomLevel, cameraPosition);
    subGrid2D->updateGrid(zoomLevel, cameraPosition);
}


void NestGrid2D::draw() {
    Shaders::flatShader->setUniform3x3f("transform", grid2D->transform);
    grid2D->draw();
    subGrid2D->draw();
}







//endregion


//region --------------------------------- Grid2D -----------------------------

Grid2D::Grid2D(std::shared_ptr<Material> material, int size, float spacing) :
    MaterialObject2D(std::move(material), std::move(material)),
    grid_size(size),
    baseSpacing(spacing)
{
    GL_TRY(glGenVertexArrays(1, &vertex_array_index));
    GL_TRY(glBindVertexArray(vertex_array_index));

    GL_TRY(glGenBuffers(1, &vertexBuffer));
    // Initial vertex buffer setup with default spacing
    updateGrid(1.0, {0, 0});
    updateVertexBuffer();

    // Vertex attribute setup
    GL_TRY(glEnableVertexAttribArray(0));
    GL_TRY(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr));
}

Grid2D::~Grid2D() {
    glDeleteVertexArrays(1, &vertex_array_index);
    glDeleteBuffers(1, &vertexBuffer);
}

void Grid2D::updateVertexBuffer() {
    std::vector<float> vertices;

    for (int i = -grid_size; i <= grid_size; ++i) {
        // Horizontal line
        vertices.push_back(-grid_size * grid_spacing + origin.x); // x1
        vertices.push_back(i * grid_spacing + origin.y);          // y1
        vertices.push_back(grid_size * grid_spacing + origin.x);  // x2
        vertices.push_back(i * grid_spacing + origin.y);          // y2

        // Vertical line
        vertices.push_back(i * grid_spacing + origin.x);          // x1
        vertices.push_back(-grid_size * grid_spacing + origin.y); // y1
        vertices.push_back(i * grid_spacing + origin.x);          // x2
        vertices.push_back(grid_size * grid_spacing + origin.y);  // y2
    }

    // Vertex Buffer setup
    GL_TRY(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer));
    GL_TRY(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW));

}

void Grid2D::draw() {
    // Store previous settings
    GLfloat previousLineWidth;
    glGetFloatv(GL_LINE_WIDTH, &previousLineWidth);

    // draw the grid
//    Shaders::flatShader->bind();
    MaterialObject2D::strokeMaterial->bind();
    glBindVertexArray(vertex_array_index);
    glLineWidth(line_width);
    glDrawArrays(GL_LINES, 0, 4 * grid_size * 2);

    // restore the previous settings
    glLineWidth(previousLineWidth);
    glBindVertexArray(0);
}

void Grid2D::updateGrid(float zoomLevel, glm::vec2 cameraPosition) {
    // Calculate new grid spacing based on zoomLevel
    bool mustUpdate = calculateNewSpacing(zoomLevel, cameraPosition);

    // Update the vertex buffer if spacing has changed significantly
    if (mustUpdate) {
        updateVertexBuffer();
    }
}

bool Grid2D::calculateNewSpacing(float zoomLevel, glm::vec2 cameraPosition) {

    double new_spacing = 0.1;
    double alpha = 1.0;
    float zoom_inverse = 1 / zoomLevel;


    double division = 5.0f;
    double curr_category = 1250;

    for (int i = 0; i < 50; ++i) {
        if (curr_category <= zoom_inverse && zoom_inverse < (curr_category * division)) {
            new_spacing = curr_category / 2.0f;
            alpha = (zoom_inverse - curr_category) / ((curr_category * division) - curr_category);
            break;
        }
        curr_category /= division;
    }

    // update opacity
    if (flipAlpha) alpha = 1.0f - alpha;
    strokeMaterial->setAlpha((float)alpha);

    new_spacing = new_spacing * baseSpacing * 2.0f;

    glm::vec2 new_origin = {
            -std::round(cameraPosition.x / new_spacing) * new_spacing,
            -std::round(cameraPosition.y / new_spacing) * new_spacing
    };

    bool result = false;

    if (new_origin != origin) {
        origin = new_origin;
        result = true;
    }

    if (new_spacing != grid_spacing) {
        grid_spacing = (float)new_spacing;
        result = true;
    }

    return result;
}

//endregion


//region -------------------------------- Camera2D ----------------------------
glm::mat3 Camera2D::getTransformation() {
    if (!shouldUpdateTransformation) return transformationMatrix;
    float cosTheta = std::cos(angle_);
    float sinTheta = std::sin(angle_);

    glm::mat3 rotate{};
    // 1. Calculate Transformation
    {
        rotate = glm::mat3(
                cosTheta, sinTheta, 0.0,
                -sinTheta, cosTheta, 0.0,
                0.0, 0.0, 1.0
        );

        glm::mat3 translate(
                1.0, 0.0, 0.0,
                0.0, 1.0, 0.0,
                position_.x, position_.y, 1.0
        );

        glm::mat3 scale(
                zoom_ / aspectRatio_, 0.0, 0.0,
                0.0, zoom_, 0.0,
                0.0, 0.0, 1.0
        );

        transformationMatrix = scale * translate * rotate;  // TODO
    }


    // 2. Calculate Inverse Transformation
    {
        glm::mat3 inverseRotate = glm::transpose(rotate);
        glm::mat3 inverseTranslate(
                1.0, 0.0, 0.0,
                0.0, 1.0, 0.0,
                -position_.x, -position_.y, 1.0
        );
        glm::mat3 inverseScale(
                1.0 / (zoom_ / aspectRatio_), 0.0, 0.0,
                0.0, 1.0 / zoom_, 0.0,
                0.0, 0.0, 1.0
        );
        inverseTransformationMatrix = inverseRotate * inverseTranslate * inverseScale;
    }

    shouldUpdateTransformation = false;
    return transformationMatrix;
}

//endregion


//region -------------------------------- Object2D ----------------------------

size_t Object::count = 0;
Object::Object() {
    count++;
    name = std::format("object {}", count);
}


//endregion


//region ---------------------------- MaterialObject2D ------------------------

MaterialObject2D::MaterialObject2D(std::shared_ptr<Material> fillMaterial, std::shared_ptr<Material> strokeMaterial)
: fillMaterial(std::move(fillMaterial)), strokeMaterial(std::move(strokeMaterial)) {}

void MaterialObject2D::setFillMaterial(std::shared_ptr<Material> mat) { fillMaterial = std::move(mat); }
std::shared_ptr<Material> MaterialObject2D::getFillMaterial() const { return fillMaterial; }

void MaterialObject2D::setStrokeMaterial(std::shared_ptr<Material> mat) { strokeMaterial = std::move(mat); }
std::shared_ptr<Material> MaterialObject2D::getStrokeMaterial() const { return strokeMaterial; }

//endregion


//region ------------------------------ Rectangle2D ---------------------------

void Rectangle2D::draw() {
    glBindVertexArray(vertex_array_index);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

    // Layout
    {
        // Enable vertex attribute array 0.
        GL_TRY(glEnableVertexAttribArray(0));
        GL_TRY(glVertexAttribPointer(
                0,              // Attribute index.
                2,              // Number of components per vertex attribute.
                GL_FLOAT,       // Data type of each component.
                GL_FALSE,       // Normalization.
                0,              // Byte offset between consecutive attributes.
                nullptr         // Offset of the first component.
        ));
    }


//    Shaders::flatShader->bind();
    if (fill_) {
        MaterialObject2D::fillMaterial->bind();
        GL_TRY(glDrawElements(GL_TRIANGLES, 6 * sizeof(unsigned int), GL_UNSIGNED_INT, nullptr));
    }

    if (drawEdges_){
        MaterialObject2D::strokeMaterial->bind();
        glDrawArrays(GL_LINE_LOOP, 0, 4);
       // glDrawElements(GL_LINES, 6 * sizeof(unsigned int), GL_UNSIGNED_INT, nullptr);
    }


    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

Rectangle2D::Rectangle2D(std::shared_ptr<Material> fillMaterial, std::shared_ptr<Material> strokeMaterial)
    : MaterialObject2D(std::move(fillMaterial), std::move(strokeMaterial)) {
    GL_TRY(glGenVertexArrays(1, &vertex_array_index));      // Generate a vertex array object.
    GL_TRY(glBindVertexArray(vertex_array_index));          // Bind the vertex array object.

    // Layout
    float x1 = -0.5, x2 = 0.5;
    float y1 = -0.5, y2 = 0.5;

    float vertices[] = {
            x1, y1,  // Bottom left
            x2, y1,  // Bottom right
            x2, y2,  // Top right
            x1, y2   // Top left
    };

    unsigned int indices[] = {
            0, 1, 2,    // First triangle
            2, 3, 0     // Second triangle
    };

    // Vertex Buffer setup
    {
        GL_TRY(glGenBuffers(1, &vertexBuffer));
        GL_TRY(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer));
        GL_TRY(glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), vertices, GL_STATIC_DRAW));
    }

    // Index buffer setup
    {
        GL_TRY(glGenBuffers(1, &indexBuffer));
        GL_TRY(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer));
        GL_TRY(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW));
    }

    // Vertex attribute setup
    {
        GL_TRY(glEnableVertexAttribArray(0));                    // Enable vertex attribute array 0.
        GL_TRY(glVertexAttribPointer(
                0,              // Attribute index.
                2,              // Number of components per vertex attribute.
                GL_FLOAT,       // Data type of each component.
                GL_FALSE,       // Normalization.
                0,              // Byte offset between consecutive attributes.
                nullptr         // Offset of the first component.
        ));
    }


}

Rectangle2D::~Rectangle2D() {
    glDeleteVertexArrays(1, &vertex_array_index);
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &indexBuffer);
}

//endregion


//region -------------------------------- Circle2D ----------------------------

Circle2D::Circle2D(std::shared_ptr<Material> fillMaterial, std::shared_ptr<Material> strokeMaterial)
        : MaterialObject2D(std::move(fillMaterial), std::move(strokeMaterial)) {
    // Generate VAO and VBO, bind them, and set up vertex data
    GL_TRY(glGenVertexArrays(1, &vertex_array_index));
    GL_TRY(glBindVertexArray(vertex_array_index));

    // Circle data
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    {
        // Center vertex for fan
        vertices.push_back(0.0f); // x
        vertices.push_back(0.0f); // y

        // Circle vertices
        for (int i = 0; i <= num_segments; ++i) {
            float theta = 2.0f * M_PI * float(i) / float(num_segments);
            float x = 0.5 * cosf(theta); // Calculate the x component
            float y = 0.5 * sinf(theta); // Calculate the y component
            vertices.push_back(x);
            vertices.push_back(y);
        }

        // Setting up indices for triangle fan
        for (unsigned int i = 0; i <= num_segments+1; i++) {
            indices.push_back(i);
        }
    }

    // Vertex Buffer setup
    {
        GL_TRY(glGenBuffers(1, &vertexBuffer));
        GL_TRY(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer));
        GL_TRY(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW));
    }

    // Index buffer setup
    {
        GL_TRY(glGenBuffers(1, &indexBuffer));
        GL_TRY(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer));
        GL_TRY(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW));
    }

    // Vertex attribute setup
    {
        GL_TRY(glEnableVertexAttribArray(0));                    // Enable vertex attribute array 0.
        GL_TRY(glVertexAttribPointer(
                0,              // Attribute index.
                2,              // Number of components per vertex attribute.
                GL_FLOAT,       // Data type of each component.
                GL_FALSE,       // Normalization.
                0,              // Byte offset between consecutive attributes.
                nullptr         // Offset of the first component.
        ));
    }
    GL_TRY(glBindVertexArray(0)); // Unbind VAO
}


void Circle2D::draw() {
    MaterialObject2D::fillMaterial->bind();

    glBindVertexArray(vertex_array_index);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

    // Layout
    {
        // Enable vertex attribute array 0.
        GL_TRY(glEnableVertexAttribArray(0));
        GL_TRY(glVertexAttribPointer(
                0,              // Attribute index.
                2,              // Number of components per vertex attribute.
                GL_FLOAT,       // Data type of each component.
                GL_FALSE,       // Normalization.
                0,              // Byte offset between consecutive attributes.
                nullptr         // Offset of the first component.
        ));
    }


//    Shaders::flatShader->bind();
    if (fill_) {
        MaterialObject2D::fillMaterial->bind();
        glDrawElements(GL_TRIANGLE_FAN, num_segments + 2, GL_UNSIGNED_INT, nullptr);
    }

    if (drawEdges_){
        MaterialObject2D::strokeMaterial->bind();
        // Start at 1 to skip the center point
        glDrawArrays(GL_LINE_LOOP, 1, num_segments);
    }


    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    glBindVertexArray(0);
}

Circle2D::~Circle2D() {
    glDeleteVertexArrays(1, &vertex_array_index);
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &indexBuffer);
}
//endregion


//region ------------------------------- Polygon2D ----------------------------

Polygon2D::Polygon2D(std::shared_ptr<Material> fillMaterial, std::shared_ptr<Material> strokeMaterial)
        : MaterialObject2D(std::move(fillMaterial), std::move(strokeMaterial)) {

    // Generate and bind VAO
    GL_TRY(glGenVertexArrays(1, &vertex_array_index));
    GL_TRY(glBindVertexArray(vertex_array_index));

    // Generate VBO and IBO
    GL_TRY(glGenBuffers(1, &vertexBuffer));
    GL_TRY(glGenBuffers(1, &indexBuffer));

}

void Polygon2D::addVertex(const glm::vec2& vertex) {
    vertices.push_back(vertex);
}


void Polygon2D::updateVertices() {

    // TODO add first as center of Polygon!

    std::vector<unsigned int> indices;
    for (size_t i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }

    // Update vertex buffer
    GL_TRY(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer));
    GL_TRY(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec2), vertices.data(), GL_STATIC_DRAW));


    // Update index buffer
    GL_TRY(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer));
    GL_TRY(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW));

    // Vertex attribute setup
    GL_TRY(glEnableVertexAttribArray(0));
    GL_TRY(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr));
}


void Polygon2D::draw() {
    MaterialObject2D::fillMaterial->bind();

    glBindVertexArray(vertex_array_index);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

    // Layout
    {
        // Enable vertex attribute array 0.
        GL_TRY(glEnableVertexAttribArray(0));
        GL_TRY(glVertexAttribPointer(
                0,              // Attribute index.
                2,              // Number of components per vertex attribute.
                GL_FLOAT,       // Data type of each component.
                GL_FALSE,       // Normalization.
                0,              // Byte offset between consecutive attributes.
                nullptr         // Offset of the first component.
        ));
    }

//    Shaders::flatShader->bind();
    if (fill_) {
        MaterialObject2D::fillMaterial->bind();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawElements(GL_TRIANGLE_FAN, vertices.size(), GL_UNSIGNED_INT, nullptr);
    }

    if (drawEdges_){
        MaterialObject2D::strokeMaterial->bind();
        // Start at 1 to skip the center point
        glDrawArrays(GL_LINE_LOOP, 0, vertices.size());
    }


    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    glBindVertexArray(0);
}

Polygon2D::~Polygon2D() {
    glDeleteVertexArrays(1, &vertex_array_index);
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &indexBuffer);
}
//endregion


//region --------------------------------- Line2D -----------------------------

void Line2D::draw() {

    glBindVertexArray(vertex_array_index);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

    // Layout
    {
        GL_TRY(glEnableVertexAttribArray(
                0));                                                    // Enable vertex attribute array 0.
        GL_TRY(glVertexAttribPointer(
                0,              // Attribute index.
                2,              // Number of components per vertex attribute.
                GL_FLOAT,       // Data type of each component.
                GL_FALSE,       // Normalization.
                0,              // Byte offset between consecutive attributes.
                nullptr  // Offset of the first component.
        ));
    }

//    Shaders::flatShader->bind();
    glDrawArrays(GL_LINES, 0, 4);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

Line2D::Line2D(std::shared_ptr<Material> material) : MaterialObject2D(std::move(material), std::move(material)) {
    GL_TRY(glGenVertexArrays(1, &vertex_array_index));      // Generate a vertex array object.
    GL_TRY(glBindVertexArray(vertex_array_index));          // Bind the vertex array object.

    float vertices[] = {
            0, 0,
            x2, y2
    };

    // Vertex Buffer
    {
        // Generate a buffer object for vertex data.
        GL_TRY(glGenBuffers(1, &vertexBuffer));
        // Bind the buffer as an array buffer.
        GL_TRY(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer));
        // Load vertex data into the buffer
        GL_TRY(glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(float), vertices, GL_STATIC_DRAW));
    }

    // Layout
    {
        GL_TRY(glEnableVertexAttribArray(0));                    // Enable vertex attribute array 0.
        GL_TRY(glVertexAttribPointer(
                0,              // Attribute index.
                2,              // Number of components per vertex attribute.
                GL_FLOAT,       // Data type of each component.
                GL_FALSE,       // Normalization.
                0,              // Byte offset between consecutive attributes.
                nullptr         // Offset of the first component.
        ));
    }
}

Line2D::~Line2D() {
    glDeleteVertexArrays(1, &vertex_array_index);    // Delete the vertex array object.
    glDeleteBuffers(1, &vertexBuffer);              // Delete the vertex buffer object.
}

//endregion


//region ---------------------------- KiwiComponent2D -------------------------

void KiwiComponent2D::draw() {
    for (const std::shared_ptr<Object2D>& obj: drawList) {
        Shaders::flatShader->setUniform3x3f("transform", obj->transform * transform);
        obj->draw();
    }
}

glm::vec4 KiwiComponent2D::getBoundingBox() {
    float minX = 100.0f;
    float minY = 100.0f;
    float maxX = -100.0f;
    float maxY = -100.0f;

    for (const std::shared_ptr<Object2D>& obj: drawList) {
        auto _transform = transform * obj->transform;
        float _obj_w2 = _transform[0][0] * 0.5f;
        float _obj_h2 = _transform[1][1] * 0.5f;
        float _obj_x = _transform[2][0];
        float _obj_y = _transform[2][1];

        float _obj_min_x = _obj_x - _obj_w2;
        float _obj_min_y = _obj_y - _obj_h2;
        float _obj_max_x = _obj_x + _obj_w2;
        float _obj_max_y = _obj_y + _obj_h2;

        if (_obj_min_x < minX) minX = _obj_min_x;
        if (_obj_min_y < minY) minY = _obj_min_y;
        if (_obj_max_x > maxX) maxX = _obj_max_x;
        if (_obj_max_y > maxY) maxY = _obj_max_y;

    }

    return {minX, minY, maxX, maxY};
}

void KiwiComponent2D::onMouseEvent(MouseEvent mouseEvent) {
    mouseEvent.DeltaPosition = oldMousePosition - mouseEvent.position;
    oldMousePosition = mouseEvent.position;

    if (mouseEvent.type == MouseEvent::Type::Enter) {
        isHovering_ = true;
        if (onMouseEnterCallback) onMouseEnterCallback(mouseEvent, *this);
    }

    else if (mouseEvent.type == MouseEvent::Type::Leave) {
        isHovering_ = false;
        if (onMouseLeaveCallback) onMouseLeaveCallback(mouseEvent, *this);
    }

    else if (mouseEvent.type == MouseEvent::Type::Click) {
        isMouseDown_ = true;
        onClickMousePosition = mouseEvent.position;
        onClickTransform = transform;
        if (onMouseClickCallback) onMouseClickCallback(mouseEvent, *this);
    }

    else if (mouseEvent.type == MouseEvent::Type::Down && onMouseDownCallback) {
        mouseEvent.onClickPosition = onClickMousePosition;
        onMouseDownCallback(mouseEvent, *this);
    }

    else if (mouseEvent.type == MouseEvent::Type::Release) {
        isMouseDown_ = false;
        if (onMouseReleaseCallback) onMouseReleaseCallback(mouseEvent, *this);
    }
}
//endregion
