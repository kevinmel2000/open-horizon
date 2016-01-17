//
// open horizon -- undefined_darkness@outlook.com
//

#include "particles.h"

#include "scene/camera.h"
#include "shared.h"

namespace renderer
{

static const int max_trail_points = 240;
static const int max_points = 120;

//------------------------------------------------------------

plane_trail::plane_trail() { m_trail_params.resize(1); }

//------------------------------------------------------------

void plane_trail::update(const nya_math::vec3 &pos, int dt)
{
    auto &trp = m_trail_params.back();

    int curr_tr_count = trp.tr.get_count();
    if (!curr_tr_count)
    {
        trp.tr.set_count(2);
        trp.tr.set(0, pos);
        trp.tr.set(1, pos);
        return;
    }

    auto diff = pos - trp.tr.get(curr_tr_count - 2).xyz();
    const float diff_len = diff.length();

    const float fragment_minimal_len = 1.0f;
    if (diff_len > fragment_minimal_len)
    {
        diff /= diff_len;

        if (diff.dot(trp.dir.get(curr_tr_count - 2).xyz()) < 1.0f)
        {
            if (curr_tr_count >= max_trail_points)
            {
                m_trail_params.resize(m_trail_params.size() + 1);

                auto &prev = m_trail_params[m_trail_params.size() - 2];
                auto &trp = m_trail_params.back();

                curr_tr_count = 2;
                trp.tr.set_count(curr_tr_count);
                trp.tr.set(0, prev.tr.get(max_trail_points-1));
                trp.dir.set_count(curr_tr_count);
                trp.dir.set(0, prev.dir.get(max_trail_points-1));
            }
            else
            {
                ++curr_tr_count;
                m_trail_params.back().tr.set_count(curr_tr_count);
                m_trail_params.back().dir.set_count(curr_tr_count);
            }
        }
    }
    else if (diff_len > 0.01f)
        diff /= diff_len;

    m_trail_params.back().tr.set(curr_tr_count - 1, pos);
    m_trail_params.back().dir.set(curr_tr_count - 1, diff, diff_len + m_trail_params.back().dir.get(curr_tr_count-2).w);
}

//------------------------------------------------------------

void explosion::update(int dt)
{
    m_time += dt * 0.001f;
}

//------------------------------------------------------------

bool explosion::is_finished() const
{
    return m_time > 1.0f;
}

//------------------------------------------------------------

void particles_render::init()
{
    auto t = shared::get_texture(shared::load_texture("Effect/Effect.nut"));

    //trails

    m_trail_tr.create();
    m_trail_dir.create();

    auto &p = m_trail_material.get_default_pass();
    p.set_shader(nya_scene::shader("shaders/plane_trail.nsh"));
    p.get_state().set_blend(true, nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha);
    p.get_state().zwrite = false;
    p.get_state().cull_face = false;
    m_trail_material.set_param_array(m_trail_material.get_param_idx("tr pos"), m_trail_tr);
    m_trail_material.set_param_array(m_trail_material.get_param_idx("tr dir"), m_trail_dir);
    m_trail_material.set_texture("diffuse", t);

    std::vector<nya_math::vec2> trail_verts(max_trail_points * 2);
    for (int i = 0; i < max_trail_points; ++i)
    {
        trail_verts[i * 2].set(-1.0f, float(i));
        trail_verts[i * 2 + 1].set(1.0f, float(i));
    }

    m_trail_mesh.set_vertex_data(trail_verts.data(), 2 * 4, (int)trail_verts.size());
    m_trail_mesh.set_vertices(0, 2);
    m_trail_mesh.set_element_type(nya_render::vbo::triangle_strip);

    //points

    struct quad_vert { float pos[2], i, tc[2]; };
    std::vector<quad_vert> verts(max_points * 4);

    for (int i = 0, idx = 0; i < (int)verts.size(); i += 4, ++idx)
    {
        verts[i+0].pos[0] = -1.0f, verts[i+0].pos[1] = -1.0f;
        verts[i+1].pos[0] = -1.0f, verts[i+1].pos[1] =  1.0f;
        verts[i+2].pos[0] =  1.0f, verts[i+2].pos[1] =  1.0f;
        verts[i+3].pos[0] =  1.0f, verts[i+3].pos[1] = -1.0f;

        for (int j = 0; j < 4; ++j)
        {
            verts[i+j].tc[0] = 0.5f * (verts[i+j].pos[0] + 1.0f);
            verts[i+j].tc[1] = 0.5f * (verts[i+j].pos[1] + 1.0f);
            verts[i+j].i = float(idx);
        }
    }

    std::vector<unsigned short> indices(max_points * 6);
    for (int i = 0, v = 0; i < (int)indices.size(); i += 6, v+=4)
    {
        indices[i] = v;
        indices[i + 1] = v + 1;
        indices[i + 2] = v + 2;
        indices[i + 3] = v;
        indices[i + 4] = v + 2;
        indices[i + 5] = v + 3;
    }


    m_point_mesh.set_vertex_data(verts.data(), sizeof(quad_vert), (unsigned int)verts.size());
    m_point_mesh.set_index_data(indices.data(), nya_render::vbo::index2b, (unsigned int)indices.size());
    m_point_mesh.set_tc(0, sizeof(float) * 3, 2);

    m_tr_pos.create();
    m_tr_tc_rgb.create();
    m_tr_tc_a.create();

    auto &p2 = m_material.get_default_pass();
    p2.set_shader(nya_scene::shader("shaders/particles.nsh"));
    p2.get_state().set_blend(true, nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha);
    p2.get_state().zwrite = false;
    p2.get_state().cull_face = false;
    m_material.set_param_array(m_material.get_param_idx("tr pos"), m_tr_pos);
    m_material.set_param_array(m_material.get_param_idx("tr tc_rgb"), m_tr_tc_rgb);
    m_material.set_param_array(m_material.get_param_idx("tr tc_a"), m_tr_tc_a);
    m_material.set_texture("diffuse", t);
}

//------------------------------------------------------------

void particles_render::draw(const plane_trail &t) const
{
    nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());

    m_trail_mesh.bind();
    for (auto &tp: t.m_trail_params)
    {
        m_trail_tr.set(tp.tr);
        m_trail_dir.set(tp.dir);
        m_trail_material.internal().set();
        m_trail_mesh.draw(tp.tr.get_count() * 2);
        m_trail_material.internal().unset();
    }
    m_trail_mesh.unbind();
}

//------------------------------------------------------------

void particles_render::draw(const explosion &e) const
{
    clear_points();
    add_point(e.m_pos, e.m_radius, tc(0, 0, 128, 128), false,
                                   tc(0, 1920, 128, 128), true);
    draw_points();
}

//------------------------------------------------------------

void particles_render::clear_points() const
{
    m_tr_pos->set_count(0);
    m_tr_tc_rgb->set_count(0);
    m_tr_tc_a->set_count(0);
}

//------------------------------------------------------------

void particles_render::add_point(const nya_math::vec3 &pos, float size, const tc &tc_rgb, bool rgb_from_a,
                                 const tc &tc_a, bool a_from_a) const
{
    const int idx = m_tr_pos->get_count();
    m_tr_pos->set_count(idx + 1);
    m_tr_tc_rgb->set_count(idx + 1);
    m_tr_tc_a->set_count(idx + 1);

    const float inv_tex_size = 1.0f / 2048.0f;
    m_tr_pos->set(idx, pos, size);
    auto tc = tc_rgb * inv_tex_size;
    if (rgb_from_a)
        tc.w = -tc.w;
    m_tr_tc_rgb->set(idx, tc);
    tc = tc_a * inv_tex_size;
    if (a_from_a)
        tc.w = -tc.w;
    m_tr_tc_a->set(idx, tc);
}

//------------------------------------------------------------

void particles_render::draw_points() const
{
    nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());

    m_point_mesh.bind();
    m_material.internal().set();
    m_point_mesh.draw(m_tr_pos->get_count() * 6);
    m_material.internal().unset();
    m_point_mesh.unbind();
}

//------------------------------------------------------------

void particles_render::release()
{
    m_trail_material.unload();
    m_material.unload();
    m_trail_mesh.release();
    m_point_mesh.release();
}

//------------------------------------------------------------
}
