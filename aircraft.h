//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "phys/plane_params.h"
#include "scene/model.h"
#include "scene/location_params.h"

//------------------------------------------------------------

class aircraft
{
public:
    bool load(const char *name, unsigned int color_idx, const location_params &params);

    void apply_location(const char *location_name, const location_params &params);

    void draw(int lod_idx);

    void set_pos(const nya_math::vec3 &pos) { m_pos = pos; m_mesh.set_pos(pos); }
    void set_rot(const nya_math::quat &rot) { m_rot = rot; m_mesh.set_rot(rot); }
    const nya_math::vec3 &get_pos() { return m_pos; }
    nya_math::quat get_rot() { return m_rot; }

    float get_speed() { return m_vel.length(); }
    float get_alt() { return m_pos.y; }
    float get_hp() { return m_hp; }
    const nya_math::vec3 &get_camera_offset() const { return m_camera_offset; }
    nya_math::vec3 get_bone_pos(const char *name);

    static unsigned int get_colors_count(const char *plane_name);

    void set_controls(const nya_math::vec3 &rot, float throttle, float brake);

    void fire_mgun() { m_controls_mgun = true; }
    void fire_rocket() { m_controls_rocket = true; }
    void change_weapon() { m_controls_special = true; }

    void set_time(unsigned int time) { m_time = time * 1000; } //in seconds

    void update(int dt);

    aircraft(): m_hp(0), m_controls_throttle(0), m_controls_brake(0), m_thrust_time(0),
    m_controls_mgun(false), m_controls_rocket(false), m_controls_special(false),
    m_special_selected(false), m_rocket_bay_time(0), m_time(0)
    {
        m_adimx_bone_idx = m_adimx2_bone_idx = -1;
        m_adimz_bone_idx = m_adimz2_bone_idx = -1;
        m_adimxz_bone_idx = m_adimxz2_bone_idx = -1;
    }

private:
    float clamp(float value, float from, float to) { if (value < from) return from; if (value > to) return to; return value; }
    float tend(float value, float target, float speed)
    {
        const float diff = target - value;
        if (diff > speed) return value + speed;
        if (-diff > speed) return value - speed;
        return target;
    }

    float m_hp;
    model m_mesh;
    float m_thrust_time;
    nya_math::vec3 m_pos;
    nya_math::vec3 m_rot_speed;
    nya_math::quat m_rot;
    nya_math::vec3 m_vel;

    nya_math::vec3 m_controls_rot;
    float m_controls_throttle;
    float m_controls_brake;
    plane_params m_params;
    bool m_controls_mgun;
    bool m_controls_rocket;
    bool m_controls_special;

    bool m_special_selected;

    float m_rocket_bay_time;

    unsigned int m_time;

    int m_adimx_bone_idx, m_adimx2_bone_idx;
    int m_adimz_bone_idx, m_adimz2_bone_idx;
    int m_adimxz_bone_idx, m_adimxz2_bone_idx;

    model m_missile;
    model m_special;

    struct wpn_mount
    {
        bool visible;
        int bone_idx;
    };

    std::vector<wpn_mount> m_msls_mount;
    std::vector<wpn_mount> m_special_mount;

    nya_math::vec3 m_camera_offset;
};

//------------------------------------------------------------