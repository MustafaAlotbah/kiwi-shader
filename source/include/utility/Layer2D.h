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

#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <map>

#include "utility/shader.h"
#include "utility/colormaps.h"

#define KIWI_API

/**
 * @brief Represents an event structure, particularly for mouse button actions.
 *
 * Contains an enumeration to represent the state of mouse buttons - None, Left, Middle, Right.
 * This structure is primarily used to track and manage mouse button events.
 */
struct Event {
    enum class MouseButton {
        None, Left, Middle, Right
    };
};

/**
 * @brief Represents a mouse event, capturing various aspects of mouse interaction.
 *
 * Stores details like the position of the mouse, the event type, and the button used.
 * It also tracks the mouse position when the button was clicked and the change in position.
 * This structure is essential for handling mouse interactions in the application.
 */
struct MouseEvent {
    glm::vec2 position;         ///< current Mouse Position
    glm::vec2 onClickPosition;  ///< position of the mouse when clicked // TODO should inherit MouseKeyDownEvent
    glm::vec2 DeltaPosition{0.0f};  ///< difference of position since last event
    float deltaWheel = 0.0;
    Event::MouseButton button;


    enum class Type {
        Click,      // first click
        Down,       // while down
        Release,    // once released
        Enter,
        Leave,
        MouseWheel,
    } type;


    MouseEvent(glm::vec2 position, Type type, Event::MouseButton button = Event::MouseButton::None)
            : position(position), onClickPosition(position), type(type), button(button) {}
};

/**
 * @brief Holds the state of mouse inputs, including the button pressed and the mouse wheel delta.
 *
 * This structure captures the current state of mouse interactions, indicating which mouse
 * button is pressed and the amount of scroll on the mouse wheel.
 *
 * @param mouseButton Represents the currently pressed mouse button, if any.
 * @param deltaMouseWheel Indicates the amount of scroll on the mouse wheel.
 */
struct InputState {

    Event::MouseButton mouseButton = Event::MouseButton::None;
    float deltaMouseWheel = 0.0f;

    InputState(Event::MouseButton button, float deltaMouseWheel)
    : mouseButton(button) , deltaMouseWheel(deltaMouseWheel)
    {}
};

/**
 * @brief Manages shader programs for rendering.
 *
 * Provides a static shared pointer to a flat shader. This class can be extended
 * to manage different shader programs used in rendering various graphical elements.
 */
class Shaders {
public:
    // predfined Shaders
    static std::shared_ptr<Shader> flatShader;  // TODO move to Shader::Flat
};

/**
 * @brief Represents a material in a graphical context, with a shader and color properties.
 *
 * This class encapsulates a shader and its associated color, allowing for binding the shader
 * with specific properties. It also allows for setting the alpha transparency and highlight intensity
 * for the material. Predefined colors and a method for initializing global materials are provided.
 *
 * @param shader Shared pointer to the shader to be used with the material.
 * @param color Color of the material, represented as a 4D vector (RGBA).
 */
class Material {
public:
    Material(std::shared_ptr<Shader> shader, const glm::vec4& color);
    virtual void bind() const;
    virtual void setAlpha(float alpha);
    virtual inline void setColor(glm::vec4 color) {this->color = color;}
    virtual void setHighlight(float highlight);

private:
    std::shared_ptr<Shader> shader;
    glm::vec4 color;
    float highlightIntensity = 0.9f;

public:
    static std::shared_ptr<Material> createFlatMaterial(const glm::vec4& color);
    static glm::vec4 getColorViridis(float value, float alpha=1.0);
    // Other methods...

    // Predefined colors
    static std::shared_ptr<Material> Black;
    static std::shared_ptr<Material> White;
    static std::shared_ptr<Material> DarkGrey;
    static std::shared_ptr<Material> Grey;
    static std::shared_ptr<Material> Blue;
    static std::shared_ptr<Material> Green;
    static std::shared_ptr<Material> Red;
    static std::shared_ptr<Material> Yellow;


    // Initialization method for global materials
    static void InitializeGlobalMaterials();
};

/**
 * @brief Base class for graphical objects.
 *
 * Provides a foundational interface for all drawable objects. It includes a unique name
 * for each object and a pure virtual draw method which must be implemented by derived classes.
 */
class Object {
protected:
    std::string name;
    static size_t count;
public:
    Object();
    virtual void draw() = 0;
    inline const char* getName() {return  name.c_str(); }

};

/**
 * @brief Represents a 2D object, inheriting from the base Object class.
 *
 * Provides a basic implementation for 2D objects, including a transformation matrix.
 * This class serves as a base for more specific 2D objects and is an extension of the
 * general Object class.
 */
class Object2D : public Object{
protected:
public:
    Object2D() = default;
   // virtual ~Object2D();
    glm::mat3 transform = glm::mat3(1.0);
};

/**
 * @brief A 2D object that includes material properties.
 *
 * Extends the Object2D class by incorporating materials for filling and stroking.
 * Provides methods to set and get these materials. This class is used for objects
 * that require material properties for rendering, such as color and texture.
 */
class MaterialObject2D : public Object2D {
protected:
    std::shared_ptr<Material> fillMaterial;
    std::shared_ptr<Material> strokeMaterial;

public:
    MaterialObject2D(std::shared_ptr<Material> fillMaterial, std::shared_ptr<Material> strokeMaterial);

    void setFillMaterial(std::shared_ptr<Material> mat) ;
    [[nodiscard]] std::shared_ptr<Material> getFillMaterial() const;

    void setStrokeMaterial(std::shared_ptr<Material> mat) ;
    [[nodiscard]] std::shared_ptr<Material> getStrokeMaterial() const;
};

/**
 * @brief A 2D rectangle object, inheriting from MaterialObject2D.
 *
 * Represents a rectangle in 2D space with customizable fill and stroke materials.
 * It maintains its own vertex array, vertex buffer, and index buffer for rendering.
 * The class provides methods for drawing the rectangle and setting its properties.
 *
 * @param fillMaterial Shared pointer to the material used for filling the rectangle.
 * @param strokeMaterial Shared pointer to the material used for the rectangle's edges.
 */
class Rectangle2D : public MaterialObject2D {
private:
    unsigned int vertex_array_index{};
    unsigned int vertexBuffer{};
    unsigned int indexBuffer{};
    bool drawEdges_ = true;
    bool fill_ = true;
public:
    void draw() override;
    explicit Rectangle2D(std::shared_ptr<Material> fillMaterial, std::shared_ptr<Material> strokeMaterial);
    ~Rectangle2D();
};

/**
 * @brief A 2D circle object, inheriting from MaterialObject2D.
 *
 * Represents a circle in 2D space with customizable fill and stroke materials.
 * It handles its own geometry creation, approximating a circle using a specified number of segments.
 * The class allows for rendering the circle with different visual properties.
 *
 * @param fillMaterial Shared pointer to the material used for filling the circle.
 * @param strokeMaterial Shared pointer to the material used for the circle's edges.
 */
class Circle2D : public MaterialObject2D {
private:
    unsigned int vertex_array_index{};
    unsigned int vertexBuffer{};
    unsigned int indexBuffer{};
    bool drawEdges_ = true;
    bool fill_ = true;
    int num_segments = 32; // Number of segments to approximate the circle

public:
    void draw() override;
    explicit Circle2D(std::shared_ptr<Material> fillMaterial, std::shared_ptr<Material> strokeMaterial);
    ~Circle2D();
};

/**
 * @brief A 2D polygon object, inheriting from MaterialObject2D.
 *
 * Represents a convex polygon in 2D space with customizable fill and stroke materials.
 * The class provides methods for defining the polygon's vertices and drawing the polygon.
 * It also handles its own vertex and index buffers for rendering.
 *
 * @param fillMaterial Shared pointer to the material used for filling the polygon.
 * @param strokeMaterial Shared pointer to the material used for the polygon's edges.
 */
class Polygon2D : public MaterialObject2D {
private:
    unsigned int vertex_array_index{};
    unsigned int vertexBuffer{};
    unsigned int indexBuffer{};
    std::vector<glm::vec2> vertices;
    bool drawEdges_ = true;
    bool fill_ = true;

public:
    Polygon2D(std::shared_ptr<Material> fillMaterial, std::shared_ptr<Material> strokeMaterial);
    ~Polygon2D();

    inline void resetVertices() {vertices.clear(); }
    void addVertex(const glm::vec2& vertex);
    void updateVertices(); // Call this after adding all vertices
    void draw() override;
};

/**
 * @brief A 2D grid object, inheriting from MaterialObject2D.
 *
 * Represents a grid in 2D space, useful for providing a reference frame or background.
 * The grid's spacing and size can be dynamically updated based on zoom level and camera position.
 * It utilizes a vertex buffer for rendering the grid lines.
 *
 * @param material Shared pointer to the material used for the grid lines.
 * @param size The size of the grid.
 * @param spacing The initial spacing between the grid lines.
 */
class Grid2D : public MaterialObject2D {
private:
    unsigned int vertex_array_index{};
    unsigned int vertexBuffer{};
    int grid_size;

    void updateVertexBuffer();
    bool calculateNewSpacing(float zoomLevel, glm::vec2 cameraPosition);
public:
    float baseSpacing = 1.0f;
    float line_width = 1.0f;
    float grid_spacing{};
    glm::vec2 origin = {0, 0};
    bool flipAlpha = false;
    Grid2D(std::shared_ptr<Material> material, int size, float spacing);
    ~Grid2D();

    void updateGrid(float zoomLevel, glm::vec2 cameraPosition);
    void draw() override;
};

/**
 * @brief A 2D nested grid object, inheriting from Object2D.
 *
 * Represents a nested grid composed of two separate Grid2D objects.
 * This class is useful for creating grids with major and minor lines, providing more visual detail.
 * It allows dynamic updates based on zoom level and camera position.
 *
 * @param size The size of the grid.
 * @param spacing The spacing for the major grid lines.
 * @param divisionBy The division factor to determine the spacing of minor grid lines.
 */
class NestGrid2D : Object2D {
public:
    NestGrid2D(int size, float spacing, float divisionBy);

    void update(float zoomLevel, glm::vec2 cameraPosition);
    void draw() override;
private:
    float spacingMajor;
    float division;

    std::shared_ptr<Grid2D> grid2D;
    std::shared_ptr<Grid2D> subGrid2D;
};

/**
 * @brief A 2D line object, inheriting from MaterialObject2D.
 *
 * Represents a simple line segment in 2D space with customizable material.
 * It maintains its own vertex array and buffer for rendering.
 *
 * @param material Shared pointer to the material used for the line.
 */
class Line2D : public MaterialObject2D {
private:
    float x2{}, y2{};
    unsigned int vertex_array_index{};
    unsigned int vertexBuffer{};

public:
    void draw() override;

    explicit Line2D(std::shared_ptr<Material> material);

    ~Line2D();
};

/**
 * @brief Abstract base class for camera implementations.
 *
 * Defines a common interface for cameras, requiring the implementation of a method
 * to set the aspect ratio of the camera view.
 */
class Camera {
public:
    virtual void setAspectRatio(float width, float height) = 0;
};

/**
 * @brief A 2D camera class for managing the view transformation in a 2D space.
 *
 * Extends the Camera class, providing functionality specific to 2D rendering.
 * It manages properties like aspect ratio, zoom, angle, and position of the camera.
 * The class offers methods to get the transformation matrix and its inverse.
 */
class Camera2D : public Camera {
private:
    friend class Layer2D; friend class KiwiLayer2D;
    [[nodiscard]] glm::mat3 getTransformation();
    [[nodiscard]] inline glm::mat3 getInverseTransformation() const { return inverseTransformationMatrix; };

public:
    Camera2D() = default;

    inline void setAspectRatio(float width, float height) override {aspectRatio_=width / height; shouldUpdateTransformation=true;}
    [[nodiscard]] inline float getAspectRatio() const {return angle_;}

    inline void setAngle(float angle) {angle_=angle; shouldUpdateTransformation=true;}
    [[nodiscard]] inline float getAngle() const {return angle_;}

    inline void setZoom(float zoom) {zoom_=zoom; shouldUpdateTransformation=true;}
    [[nodiscard]] inline float getZoom() const {return zoom_;}

    inline void setPosition(glm::vec2 position) {position_=position; shouldUpdateTransformation=true;}
    [[nodiscard]] inline glm::vec2 getPosition() const {return position_;}

private:
    float angle_ = 0.0;
    float zoom_ = 0.16;
    float aspectRatio_ = 1.0;
    glm::vec2 position_{};
private:
    bool shouldUpdateTransformation = true;
    glm::mat3 transformationMatrix{};
    glm::mat3 inverseTransformationMatrix{};
};

/**
 * @brief A 2D component class inheriting from Object2D, capable of handling mouse events.
 *
 * Provides a base for creating interactive 2D components. It supports mouse event handling
 * and allows adding child objects. The class can be extended to create various interactive elements.
 */
class KiwiComponent2D : public Object2D {
public:
    // Constructor
    KiwiComponent2D() = default;

    // Destructor
    virtual ~KiwiComponent2D() = default;

    // Override the draw method from Object2D
    void draw() override;

    // Event handling methods
    virtual glm::vec4 getBoundingBox();

    void onMouseEvent(MouseEvent mouseEvent);

    [[nodiscard]] inline bool isHovering() const {return isHovering_; }
    [[nodiscard]] inline bool isMouseDown() const {return isMouseDown_; }

    inline void add(const std::shared_ptr<Object2D>& obj) {drawList.push_back(obj); }
    inline void onMouseEnter(const std::function<void(MouseEvent, KiwiComponent2D&)>& callback) { onMouseEnterCallback=callback; };
    inline void onMouseLeave(const std::function<void(MouseEvent, KiwiComponent2D&)>& callback) { onMouseLeaveCallback=callback; };
    inline void onMouseClick(const std::function<void(MouseEvent, KiwiComponent2D&)>& callback) { onMouseClickCallback=callback; };
    inline void onMouseDown(const std::function<void(MouseEvent, KiwiComponent2D&)>& callback) { onMouseDownCallback=callback; };
    inline void onMouseRelease(const std::function<void(MouseEvent, KiwiComponent2D&)>& callback) { onMouseReleaseCallback=callback; };

public: //Getters
    [[nodiscard]] inline glm::mat3 getOnClickTransform() const {return onClickTransform; }

private:
    // Callback functions for events
    std::function<void(MouseEvent, KiwiComponent2D&)> onMouseEnterCallback;
    std::function<void(MouseEvent, KiwiComponent2D&)> onMouseLeaveCallback;
    std::function<void(MouseEvent, KiwiComponent2D&)> onMouseClickCallback;
    std::function<void(MouseEvent, KiwiComponent2D&)> onMouseDownCallback;
    std::function<void(MouseEvent, KiwiComponent2D&)> onMouseReleaseCallback;
private:
    std::vector<std::shared_ptr<Object2D>> drawList;
    bool isHovering_ = false;
    bool isMouseDown_ = false;
    glm::vec2 onClickMousePosition{0.0};
    glm::vec2 oldMousePosition{0.0};
    glm::mat3 onClickTransform{0.0};    ///< temporary transform when the mouse clicked, useful for changing transform
};

/**
 * @brief Abstract base class for a layer in the rendering stack.
 *
 * Defines a common interface for layers, requiring implementations for rendering,
 * handling mouse events, and managing the camera. It acts as a foundational element
 * for creating complex layered renderings.
 */
class KiwiLayer {
public:
    virtual void render(float windowWidth, float windowHeight, double time, double deltaTime) = 0;
    virtual ~KiwiLayer() = default;
    virtual inline Camera& getCamera() = 0;
    virtual void updateMousePosition(glm::vec2 normalizedPosition) = 0;
    virtual void handleMouseEvent(MouseEvent mouseEvent) = 0;
};

/**
 * @brief A specific implementation of KiwiLayer for 2D rendering.
 *
 * Manages the rendering of 2D objects, handling of mouse events, and the camera
 * specific to a 2D context. It allows adding 2D objects and components to the layer.
 */
class KiwiLayer2D : public KiwiLayer {
public:
    KiwiLayer2D();
    // override methods
    ~KiwiLayer2D() override = default;
    void render(float windowWidth, float windowHeight, double time, double deltaTime) override;
    inline Camera2D& getCamera() override {return camera;};
    void updateMousePosition(glm::vec2 normalizedPosition) override;
    // other public methods (for API)
    inline NestGrid2D& getGrid() {return grid_;};
    inline void add(const std::shared_ptr<Object2D>& obj) {drawList.push_back(obj); }
    inline std::vector<std::shared_ptr<Object2D>> getItems() {return drawList; }
    inline std::shared_ptr<Object2D> getItem(int i) {return drawList[i]; }
    inline void onMouseClick(const std::function<bool(MouseEvent, KiwiLayer2D&)>& callback) { onMouseClickCallback=callback; };
    inline void onMouseDown(const std::function<bool(MouseEvent, KiwiLayer2D&)>& callback) { onMouseDownCallback=callback; };
    inline void onMouseRelease(const std::function<bool(MouseEvent, KiwiLayer2D&)>& callback) { onMouseReleaseCallback=callback; };

    [[maybe_unused]] inline glm::vec2 getMouseCoordinates() {return mousePosition; }

    void registerComponent(std::shared_ptr<KiwiComponent2D> component);
    void handleMouseEvent(MouseEvent mouseEvent) override;
private:
    std::vector<std::shared_ptr<Object2D>> drawList;
    std::vector<std::shared_ptr<KiwiComponent2D>> components;    // TODO abstract to KiwiComponent
    std::function<bool(MouseEvent, KiwiLayer2D&)> onMouseClickCallback;
    std::function<bool(MouseEvent, KiwiLayer2D&)> onMouseDownCallback;
    std::function<bool(MouseEvent, KiwiLayer2D&)> onMouseReleaseCallback;
private:
    Camera2D camera;
    NestGrid2D grid_;
    glm::vec2 mousePosition{};    ///< mouse position w.r.t. camera

    glm::vec2 prevMousePosition{};    ///< This for canvas translation
    glm::vec2 prevCameraPosition{};    ///< This for canvas translation

    glm::vec2 mouseNormalizedPosition{};    ///< mouse position w.r.t. frame
    friend class KiwiCore;
};

/**
 * @brief Manages a frame buffer and its associated properties for rendering.
 *
 * Handles the creation and management of a frame buffer, including its size, texture, and rendering.
 * Provides functionalities for resizing the frame, binding it for rendering, and saving the frame as an image.
 */
class KiwiFrame {
public:
    KiwiFrame();
    ~KiwiFrame();
    void resize(int width, int height);
    [[nodiscard]] inline glm::vec2 size() const {return frameSize; }
    void bind() const;
    static void unbind() ;
    [[nodiscard]] inline void *getTextureId() const { return reinterpret_cast<void *>(textureId); }
    [[maybe_unused]] void saveFrameAsImage(const std::string& filename);
private:
    glm::vec2 frameSize = {1280, 920};
    unsigned int textureId{};
    unsigned int frameBufferObject{};   // color values
    unsigned int renderBufferObject{};  // depth values
};

/**
 * @brief Represents the state of the Kiwi application, capturing essential user input and positioning data.
 *
 * This structure is designed to encapsulate the current state of user interaction within the application,
 * particularly focusing on mouse-related events and positioning. It is utilized within the KiwiCore class
 * to manage and respond to user inputs effectively across different components and layers of the application.
 *
 * @param normalizedMousePos Represents the mouse position normalized with respect to the frame.
 * @param origin The origin point for calculating positions and transformations within the application.
 * @param input An instance of InputState, capturing the current state of mouse interactions including
 *              the mouse button pressed and the scroll amount on the mouse wheel.
 */
struct KiwiState {
    glm::vec2 normalizedMousePos{};
    glm::vec2 origin{};
    InputState input{Event::MouseButton::None,0.0};
};

/**
 * @brief Core class for managing the application's rendering pipeline and state.
 *
 * Acts as the central manager for the application, handling events, rendering frames,
 * and maintaining the application state. It supports adding layers and managing their interactions.
 */
class KiwiCore {
public:
    KiwiCore();
    ~KiwiCore() = default;
    void pollEvents(glm::vec2 windowPos, glm::vec2 mousePos, InputState inputState);
    void renderFrame(float windowWidth, float windowHeight, double time, double deltaTime);

    [[nodiscard]] inline void *getTextureId() const { return frame.getTextureId(); }
public:

    [[nodiscard]] inline glm::vec2 frameSize() const {return frame.size(); }
public: // API overrides
    virtual void onLoad() = 0;
    virtual void onUpdate(float time, float deltaTime) = 0;
    virtual void onUpdateUI() = 0;
    virtual void loadShaderFromMenu(const std::string& path) { /* Default: do nothing */ }
    virtual void onMouseButton(int button, int action, double x, double y, GLFWwindow* window) { /* Default: do nothing */ }
    virtual void onMouseMove(double x, double y) { /* Default: do nothing */ }
    virtual void onMouseScroll(double yOffset) { /* Default: do nothing */ }
public:
    KIWI_API void addLayer(std::shared_ptr<KiwiLayer> layer);
private:
    void calcNormalizedMousePos(glm::vec2 windowPos, glm::vec2 mousePos);

protected:
    KiwiFrame frame;
    std::vector<std::shared_ptr<KiwiLayer>> layers;
    KiwiState state;
};

/**
 * @brief Factory class for creating and managing KiwiCore applications.
 *
 * Provides a singleton pattern for registering and creating different KiwiCore applications.
 * It maintains a map of constructors, allowing the creation of specific application instances by name.
 */
class KiwiAppFactory {
public:
    static KiwiAppFactory& getInstance() {
        static KiwiAppFactory instance;
        return instance;
    }

    bool registerApp(const std::string& name, const std::function<KiwiCore*()>& constructor) {
        constructors[name] = constructor;
        return true;
    }

    KiwiCore* createApp(const std::string& name) {
        auto it = constructors.find(name);
        return it == constructors.end() ? nullptr : (it->second)();
    }

private:
    std::map<std::string, std::function<KiwiCore*()>> constructors;
};
