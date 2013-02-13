/*
**  ClanLib SDK
**  Copyright (c) 1997-2012 The ClanLib Team
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
**  Note: Some of the libraries ClanLib may link to may have additional
**  requirements or restrictions.
**
**  File Author(s):
**
**    Magnus Norddahl
*/

#pragma once

#include "Culling/visible_object.h"
#include <list>
namespace clan
{

class Scene;
class SceneParticleEmitter_Impl;
class OctTreeObject;
class AxisAlignedBoundingBox;
class ParticleEmitterPassData;

class SceneParticleEmitter
{
public:
	SceneParticleEmitter();
	SceneParticleEmitter(Scene *scene);
	bool is_null() const { return !impl; }

	enum Type
	{
		type_omni,
		type_spot
	};

	Type get_type() const;
	clan::Vec3f get_position() const;
	clan::Quaternionf get_orientation() const;
	float get_particles_per_second() const;
	float get_falloff() const;
	float get_life_span() const;
	float get_start_size() const;
	float get_end_size() const;
	float get_speed() const;
	clan::Vec3f get_acceleration() const;
	std::string get_particle_texture() const;
	std::string get_gradient_texture() const;

	void set_type(Type type);
	void set_position(const clan::Vec3f &position);
	void set_orientation(const clan::Quaternionf &orientation);
	void set_particles_per_second(float pps);
	void set_falloff(float falloff);
	void set_life_span(float life_span);
	void set_start_size(float size);
	void set_end_size(float size);
	void set_speed(float speed);
	void set_acceleration(const clan::Vec3f &acceleration);
	void set_particle_texture(const std::string &texture);
	void set_gradient_texture(const std::string &texture);

private:
	std::shared_ptr<SceneParticleEmitter_Impl> impl;
};

class SceneParticleEmitter_Impl : public VisibleObject
{
public:
	SceneParticleEmitter_Impl(Scene *scene);
	~SceneParticleEmitter_Impl();

	AxisAlignedBoundingBox get_aabb();

	Scene *scene;
	OctTreeObject *tree_object;
	std::list<SceneParticleEmitter_Impl *>::iterator it;

	SceneParticleEmitter::Type type;
	clan::Vec3f position;
	clan::Quaternionf orientation;

	float particles_per_second;
	float falloff;
	float life_span;
	float start_size;
	float end_size;
	float speed;
	clan::Vec3f acceleration;
	std::string particle_texture;
	std::string gradient_texture;

	std::shared_ptr<ParticleEmitterPassData> pass_data;
};

class SceneParticleEmitterVisitor
{
public:
	virtual void emitter(clan::GraphicContext &gc, const clan::Mat4f &world_to_eye, const clan::Mat4f &eye_to_projection, SceneParticleEmitter_Impl *emitter) = 0;
};

}
