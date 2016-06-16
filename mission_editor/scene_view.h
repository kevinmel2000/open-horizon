//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include <QGLWidget>
#include "renderer/location.h"
#include "renderer/model.h"
#include "phys/physics.h"
#include "render/debug_draw.h"
#include "game/objects.h"

//------------------------------------------------------------

class scene_view : public QGLWidget
{
    Q_OBJECT

public:
    scene_view(QWidget *parent = NULL);

public:
    void load_location(std::string name);

    enum mode
    {
        mode_add,
        mode_edit,
        mode_path,
        mode_zone,
        mode_other
    };

    void set_mode(mode m) { m_mode = m; }
    void set_selected_add(std::string str);

    struct object
    {
        std::string id;
        nya_math::vec3 pos;
        nya_math::angle_deg yaw;
        float y = 0;
    };

    const std::vector<object> get_objects() const { return m_objects; }

private:
    virtual void initializeGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void paintGL() override;

private:
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;

private:
    nya_math::vec3 world_cursor_pos() const;
    void draw(const object &o);

private:
    renderer::location m_location;
    phys::world m_location_phys;
    nya_render::debug_draw m_dd;
    nya_math::angle_deg m_camera_yaw, m_camera_pitch;
    nya_math::vec3 m_camera_pos;
    int m_mouse_x = 0, m_mouse_y = 0;
    nya_math::vec3 m_cursor_pos;
    mode m_mode;
    object m_selected_add;
    std::map<std::string, renderer::model> m_models;
    std::vector<object> m_objects;
};

//------------------------------------------------------------
