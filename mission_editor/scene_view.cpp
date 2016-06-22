//
// open horizon -- undefined_darkness@outlook.com
//

#include "scene_view.h"
#include "render/render.h"
#include "scene/camera.h"
#include "renderer/shared.h"
#include <QMouseEvent>
#include <QWheelEvent>

//------------------------------------------------------------

static const float location_size = 256 * 256.0f;

//------------------------------------------------------------

static const nya_math::vec4 white = nya_math::vec4(255,255,255,255)/255.0;
static const nya_math::vec4 green = nya_math::vec4(103,223,144,255)/255.0;
static const nya_math::vec4 red = nya_math::vec4(200,0,0,255)/255.0;
static const nya_math::vec4 blue = nya_math::vec4(100,200,200,255)/255.0;

//------------------------------------------------------------

void scene_view::load_location(std::string name)
{
    context()->makeCurrent();
    m_location = renderer::location();
    shared::clear_textures();
    m_location.load(name.c_str());
    m_location_phys.set_location(name.c_str());
    m_models.clear();
    set_selected_add(m_selected_add.id);
    m_camera_pos.set(0, 1000, 0);
    m_camera_yaw = 0;
    m_camera_pitch = 30;
}

//------------------------------------------------------------

void scene_view::set_selected_add(std::string str)
{
    m_selected_add.id = str;
    cache_mesh(str);
}

//------------------------------------------------------------

void scene_view::add_object(const object &o)
{
    m_objects.push_back(o);
    cache_mesh(o.id);
}

//------------------------------------------------------------

void scene_view::cache_mesh(std::string str)
{
    if (str.empty())
        return;

    auto &objects = game::get_objects_list();
    for (auto &o: objects)
    {
        if (o.id == str)
        {
            if (m_models.find(str) == m_models.end())
                m_models[str].load(o.model.c_str(), m_location.get_params());

            if (str == m_selected_add.id)
                m_selected_add.y = o.y, m_selected_add.dy = o.dy;
            return;
        }
    }
}

//------------------------------------------------------------


void scene_view::clear_selection()
{
    for (auto &s: m_selection)
        s.second.clear();
}

//------------------------------------------------------------

void scene_view::select(std::string group, int idx)
{
    m_selection[group].insert(idx);
}

//------------------------------------------------------------

void scene_view::set_focus(std::string group, int idx)
{
    object *o = 0;
    if (group == "objects" && idx < m_objects.size())
        o = &m_objects[idx];
    else if (group == "player spawn")
        o = &m_player;

    if (!o)
        return;

    nya_math::vec2 dpos(0.0f, 100.0f);
    dpos.rotate(m_camera_yaw);

    m_camera_pos = o->pos + nya_math::vec3(dpos.x, o->y + 50.0, dpos.y);
    m_camera_pitch = 30;

    update();
}

//------------------------------------------------------------

void scene_view::delete_selected()
{
    if (m_mode != mode_edit)
        return;

    const auto &sel_objects = m_selection["objects"];
    for (auto o = sel_objects.rbegin(); o != sel_objects.rend(); ++o)
    {
        if (*o >= m_objects.size())
            continue;

        m_objects.erase(m_objects.begin() + *o);
    }

    update_objects_tree();
    update();
}

//------------------------------------------------------------

scene_view::scene_view(QWidget *parent): QGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

//------------------------------------------------------------

void scene_view::initializeGL()
{
    nya_render::set_clear_color(0.2f,0.4f,0.5f,0.0f);
    nya_render::apply_state(true);
    m_dd.set_point_size(4.0f);
    m_dd.set_line_width(1.5f);
}

//------------------------------------------------------------

void scene_view::resizeGL(int w, int h)
{
    nya_render::set_viewport(0, 0, w, h);
    nya_scene::get_camera_proxy()->set_proj(60.0, w/float(h), 1.0f, 100000.0f);
}

//------------------------------------------------------------

void scene_view::paintGL()
{
    const float height = m_location_phys.get_height(m_camera_pos.x, m_camera_pos.z, false);

    nya_scene::get_camera_proxy()->set_rot(m_camera_yaw, m_camera_pitch, 0.0f);
    nya_scene::get_camera_proxy()->set_pos(m_camera_pos + nya_math::vec3(0, height, 0));

    nya_render::clear(true, true);
    m_location.update_tree_texture();
    m_location.draw();

    for (auto &o: m_objects)
        draw(o);

    m_cursor_pos = world_cursor_pos();

    if (m_mode == mode_add)
    {
        m_selected_add.pos = m_cursor_pos;
        draw(m_selected_add);
    }

    m_dd.clear();

    if (m_mode == mode_add || m_mode == mode_edit)
    {
        const auto &sel_objects = m_selection["objects"];
        int idx = 0;
        for (auto &o: m_objects)
        {
            auto color = (m_mode == mode_edit && sel_objects.find(idx++) != sel_objects.end()) ? red : green;
            m_dd.add_line(o.pos, o.pos + nya_math::vec3(0, o.y, 0), color);
            m_dd.add_point(o.pos, color);
        }
    }

    if (m_mode == mode_add)
    {
        m_dd.add_line(m_cursor_pos, m_cursor_pos + nya_math::vec3(0, m_selected_add.y, 0), red);
        m_dd.add_point(m_cursor_pos, red);
    }

    auto color = (m_mode == mode_edit && !m_selection["player spawn"].empty()) ? red : blue;
    auto player_up = m_player.pos + nya_math::vec3(0, m_player.y, 0);
    m_dd.add_line(m_player.pos, player_up, color);
    m_dd.add_point(m_player.pos, color);
    auto player_fw = nya_math::vec2(0.0f, 1.0f).rotate(-m_player.yaw) * 4.0f;
    auto player_fw3 = nya_math::vec3(player_fw.x, 0, player_fw.y) * 2.0;
    m_dd.add_line(player_up - player_fw3, player_up + player_fw3, color);
    nya_math::vec3 player_r3(player_fw.y, 0, -player_fw.x);
    m_dd.add_line(player_up + player_fw3, player_up + player_r3, color);
    m_dd.add_line(player_up + player_fw3, player_up - player_r3, color);

    nya_render::set_state(nya_render::state());
    nya_render::depth_test::disable();
    nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());
    m_dd.draw();
}

//------------------------------------------------------------

void scene_view::draw(const object &o)
{
    if (o.id.empty())
        return;

    auto it = m_models.find(o.id);
    if (it == m_models.end())
        return;

    auto &m = it->second;
    m.set_pos(o.pos + nya_math::vec3(0, o.y + o.dy, 0));
    m.set_rot(nya_math::quat(nya_math::angle_deg(), o.yaw, 0.0f));
    m.draw(0);
}

//------------------------------------------------------------

nya_math::vec3 scene_view::world_cursor_pos() const
{
    auto r = nya_render::get_viewport();
    nya_math::vec4 pos(m_mouse_x, r.height - m_mouse_y, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glReadPixels(pos.x, pos.y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &pos.z);
    nya_render::apply_state(true);

    pos.x = 2.0 * pos.x / r.width - 1.0f;
    pos.y = 2.0 * pos.y / r.height - 1.0f;
    pos.z = 2.0 * pos.z - 1.0;

    auto mvp = nya_scene::get_camera().get_view_matrix() * nya_scene::get_camera().get_proj_matrix();
    mvp.invert();

    pos = mvp * pos;
    pos /= pos.w;

    return pos.xyz();
}

//------------------------------------------------------------

template<typename t> std::string new_name(std::string s, const std::vector<t> &objects)
{
    for (size_t i = 0; i < objects.size() + 1; ++i)
    {
        std::string name = s + std::to_string(i+1);
        if (std::find_if(objects.begin(), objects.end(), [name](const t &o){ return o.name == name; }) == objects.end())
            return name;
    }

    return s;
}

//------------------------------------------------------------

void scene_view::mousePressEvent(QMouseEvent *event)
{
    auto btns = event->buttons();
    const bool mouse_left = btns.testFlag(Qt::LeftButton);

    if (m_mode == mode_add && mouse_left && !m_selected_add.id.empty())
    {
        m_objects.push_back(m_selected_add);
        m_objects.back().name = new_name("object", m_objects);
        update_objects_tree();
    }

    m_mouse_x = event->localPos().x();
    m_mouse_y = event->localPos().y();

    update();
}

//------------------------------------------------------------

void scene_view::mouseMoveEvent(QMouseEvent *event)
{
    int x = event->localPos().x();
    int y = event->localPos().y();

    const bool shift = event->modifiers().testFlag(Qt::ShiftModifier);
    const bool alt = event->modifiers().testFlag(Qt::AltModifier);

    bool lock_mouse = false;

    auto btns = event->buttons();
    if (btns.testFlag(Qt::RightButton))
    {
        if (shift)
        {
            m_camera_yaw += x - m_mouse_x;
            m_camera_pitch += y - m_mouse_y;
            m_camera_yaw.normalize();
            m_camera_pitch.clamp(-90, 90);
        }
        else if (alt)
        {
            m_camera_pos.y -= (y - m_mouse_y) * 10.0f;
            m_camera_pos.y = nya_math::clamp(m_camera_pos.y, 20.0f, 5000.0f);
        }
        else
        {
            nya_math::vec2 dpos(x - m_mouse_x, y - m_mouse_y);
            dpos.rotate(m_camera_yaw);
            dpos *= m_camera_pos.y / 30.0f;
            m_camera_pos.x += dpos.x, m_camera_pos.z += dpos.y;
            m_camera_pos.x = nya_math::clamp(m_camera_pos.x, -location_size, location_size);
            m_camera_pos.z = nya_math::clamp(m_camera_pos.z, -location_size, location_size);
        }
    }
    else if (btns.testFlag(Qt::LeftButton))
    {
        nya_math::vec2 dpos(x - m_mouse_x, y - m_mouse_y);
        dpos.rotate(m_camera_yaw);
        dpos *= m_camera_pos.y / 300.0f;
        if (shift)
            dpos *= 10.0f;

        if (m_mode == mode_edit)
        {
            for (auto &o: m_selection["objects"])
            {
                if (o >= m_objects.size())
                    continue;

                auto &obj = m_objects[o];
                obj.pos.x += dpos.x, obj.pos.z += dpos.y;
                obj.pos.y = m_location_phys.get_height(obj.pos.x, obj.pos.z, true);
            }

            if (!m_selection["player spawn"].empty())
            {
                m_player.pos.x += dpos.x, m_player.pos.z += dpos.y;
                m_player.pos.y = m_location_phys.get_height(m_player.pos.x, m_player.pos.z, true);
            }
        }
    }
    else if (alt)
    {
        const float add = -(y - m_mouse_y) * (shift ? 10.0f : 1.0f);
        lock_mouse = true;

        if (m_mode == mode_add)
            m_selected_add.y = nya_math::clamp(m_selected_add.y + add, 0.0f, 10000.0f);

        if (m_mode == mode_edit)
        {
            for (auto &o: m_selection["objects"])
            {
                if (o < m_objects.size())
                    m_objects[o].y = nya_math::clamp(m_objects[o].y + add, 0.0f, 10000.0f);
            }

            if (!m_selection["player spawn"].empty())
                m_player.y = nya_math::clamp(m_player.y + add, 0.0f, 10000.0f);
        }
    }
    else if (shift)
    {
        const float add = (y - m_mouse_y) * 4.0f;
        lock_mouse = true;

        if (m_mode == mode_add)
            m_selected_add.yaw = (m_selected_add.yaw + add).normalize();

        if (m_mode == mode_edit)
        {
            for (auto &o: m_selection["objects"])
            {
                if (o < m_objects.size())
                    m_objects[o].yaw = (m_objects[o].yaw + add).normalize();
            }

            if (!m_selection["player spawn"].empty())
                m_player.yaw = (m_player.yaw + add).normalize();
        }
    }

    if (lock_mouse)
    {
        QPoint glob = mapToGlobal(QPoint(m_mouse_x, m_mouse_y));
        clearFocus();
        QCursor::setPos(glob);
        setFocus();
    }
    else
        m_mouse_x = x, m_mouse_y = y;

    update();
}

//------------------------------------------------------------

void scene_view::wheelEvent(QWheelEvent *event)
{
    m_camera_pos.y += event->delta();
    m_camera_pos.y = nya_math::clamp(m_camera_pos.y, 20.0f, 5000.0f);
    update();
}

//------------------------------------------------------------