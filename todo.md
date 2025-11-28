# Project TODOs

---

### 1. Logging and Error Handling
- [x] Implement a global logger for consistent logging across the application.
- [ ] Develop an error handler for robust error management.
- [ ] Multi-threading safe
- [ ] File logging
- [ ] Filtering (DEBUG, INFO, WARN, ERROR)
- [ ] Collapsible long messages

### Refactoring
- [ ] Scene Graph
- [X] Event System
- [ ] Separate Implementation (OpenGL /KiwiGL)

### 2. Drawing Primitives Expansion
- [X] Add support for drawing circles.
- [ ] Add support for drawing custom shaders.
- [ ] Implement ellipse drawing capabilities.
- [X] Integrate polygon rendering.
- [ ] Enable mesh creation and rendering.

### 3. Camera and Environment Enhancements
- [ ] Enhance the camera system for more advanced controls.
- [ ] Develop a 3D environment to extend beyond 2D rendering.

### 4. Advanced Rendering Features
- [ ] Integrate texture support for richer visuals.
- [ ] Implement batch rendering for improved performance.

### 5. Resource Management
- [ ] Develop a resource manager for efficient handling of assets.

### 6. UI Enhancements
- [ ] Enhance the UI theme for better user experience and aesthetics.

### 7. Full Object System
- [ ] Create a comprehensive object system with various capabilities:
    - **Drawable Interface**
        - [ ] `.draw` method for rendering the object.
    - **Mouse Interaction**
        - [X] `Hoverable` with `.onHover`, `.onMouseEnter`, and `.onMouseLeave` methods.
        - [X] `Clickable` with `.onClick` method for mouse click interactions.
    - **Selection Handling**
        - [ ] `Selectable` with `.onSelect` method for object selection.

### 8. Display Mode

- [ ] Editable or Read Only