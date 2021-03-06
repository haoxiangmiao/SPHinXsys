/* -------------------------------------------------------------------------*
*								SPHinXsys									*
* --------------------------------------------------------------------------*
* SPHinXsys (pronunciation: s'finksis) is an acronym from Smoothed Particle	*
* Hydrodynamics for industrial compleX systems. It provides C++ APIs for	*
* physical accurate simulation and aims to model coupled industrial dynamic *
* systems including fluid, solid, multi-body dynamics and beyond with SPH	*
* (smoothed particle hydrodynamics), a meshless computational method using	*
* particle discretization.													*
*																			*
* SPHinXsys is partially funded by German Research Foundation				*
* (Deutsche Forschungsgemeinschaft) DFG HU1527/6-1, HU1527/10-1				*
* and HU1527/12-1.															*
*                                                                           *
* Portions copyright (c) 2017-2020 Technical University of Munich and		*
* the authors' affiliations.												*
*                                                                           *
* Licensed under the Apache License, Version 2.0 (the "License"); you may   *
* not use this file except in compliance with the License. You may obtain a *
* copy of the License at http://www.apache.org/licenses/LICENSE-2.0.        *
*                                                                           *
* --------------------------------------------------------------------------*/
/**
* @file 	particle_dynamics_algorithms.h
* @brief 	This is the classes for algorithms particle dynamics.
* @detail	Generally, there are four types dynamics. One is without particle interaction.
*			One is with particle interaction within a body. One is with particle interaction
*			between a center body and other contacted bodies. 
* 			Still another is the combination of the last two. 
*			For the first dynamics, there is also reduce dynamics 
*			which carries reduced operations through the particles of the body.

* @author	Chi ZHang and Xiangyu Hu
* @version	0.1
*/
#pragma once

#include "base_particle_dynamics.h"
#include "base_particle_dynamics.hpp"

namespace SPH 
{
	/**
	* @class ParticleDynamicsSimple
	* @brief Simple particle dynamics without considering particle interaction
	*/
	template <class BodyType, class ParticlesType = BaseParticles, class MaterialType = BaseMaterial>
	class ParticleDynamicsSimple : public ParticleDynamics<void, BodyType, ParticlesType, MaterialType>
	{
	protected:
		virtual void Update(size_t index_particle_i, Real dt = 0.0) = 0;
		InnerFunctor functor_update_;
	public:
		explicit ParticleDynamicsSimple(SPHBody* body)
			: ParticleDynamics<void, BodyType, ParticlesType, MaterialType>(body),
			functor_update_(std::bind(&ParticleDynamicsSimple::Update, this, _1, _2)) {};
		virtual ~ParticleDynamicsSimple() {};

		virtual void exec(Real dt = 0.0) override;
		virtual void parallel_exec(Real dt = 0.0) override;
	};

	/**
	* @class ParticleDynamicsReduce
	* @brief Base abstract class for reduce
	*/
	template <class ReturnType, typename ReduceOperation,
		class BodyType, class ParticlesType = BaseParticles, class MaterialType = BaseMaterial>
		class ParticleDynamicsReduce : public ParticleDynamics<ReturnType, BodyType, ParticlesType, MaterialType>
	{
	protected:
		ReduceOperation reduce_operation_;

		/** inital or reference value */
		ReturnType initial_reference_;
		virtual void SetupReduce() {};
		virtual ReturnType ReduceFunction(size_t index_particle_i, Real dt = 0.0) = 0;
		virtual ReturnType OutputResult(ReturnType reduced_value) { return reduced_value; };
		ReduceFunctor<ReturnType> functor_reduce_function_;
	public:
		explicit ParticleDynamicsReduce(SPHBody* body)
			: ParticleDynamics<ReturnType, BodyType, ParticlesType, MaterialType>(body),
			functor_reduce_function_(std::bind(&ParticleDynamicsReduce::ReduceFunction, this, _1, _2)),
			initial_reference_() {};
		virtual ~ParticleDynamicsReduce() {};

		virtual ReturnType exec(Real dt = 0.0) override;
		virtual ReturnType parallel_exec(Real dt = 0.0) override;
	};

	/**
	* @class ParticleDynamicsInner
	* @brief This is the class for inner interactions
	* in which one the particles from the same body
	* interact with each other
	*/
	template <class BodyType, class ParticlesType = BaseParticles, class MaterialType = BaseMaterial>
	class ParticleDynamicsInner : public ParticleDynamicsWithInnerConfigurations<BodyType, ParticlesType, MaterialType>
	{
	protected:
		virtual void InnerInteraction(size_t index_particle_i, Real dt = 0.0) = 0;
		InnerFunctor functor_inner_interaction_;
	public:
		explicit ParticleDynamicsInner(SPHBodyInnerRelation* body_inner_relation) 
			: ParticleDynamicsWithInnerConfigurations<BodyType, ParticlesType, MaterialType>(body_inner_relation),
			functor_inner_interaction_(std::bind(&ParticleDynamicsInner::InnerInteraction, this, _1, _2)) {};
		virtual ~ParticleDynamicsInner() {};

		virtual void exec(Real dt = 0.0) override;
		virtual void parallel_exec(Real dt = 0.0) override;
	};

	/**
	* @class ParticleDynamicsInnerWithUpdate
	* @brief This class includes an initialization, an inner interaction and an update steps
	*/
	template <class BodyType, class ParticlesType, class MaterialType = BaseMaterial>
	class ParticleDynamicsInnerWithUpdate
		: public ParticleDynamicsInner<BodyType, ParticlesType, MaterialType>
	{
	protected:
		virtual void Update(size_t index_particle_i, Real dt = 0.0) = 0;
		InnerFunctor functor_update_;
	public:
		ParticleDynamicsInnerWithUpdate(SPHBodyInnerRelation* body_inner_relation)
			: ParticleDynamicsInner<BodyType, ParticlesType, MaterialType>(body_inner_relation),
			functor_update_(std::bind(&ParticleDynamicsInnerWithUpdate::Update, this, _1, _2)) {}
		virtual ~ParticleDynamicsInnerWithUpdate() {};

		virtual void exec(Real dt = 0.0) override;
		virtual void parallel_exec(Real dt = 0.0) override;
	};

	/**
	* @class ParticleDynamicsInner1Level
	* @brief This class includes an initialization, an inner interaction and an update steps
	*/
	template <class BodyType, class ParticlesType, class MaterialType = BaseMaterial>
	class ParticleDynamicsInner1Level 
		: public ParticleDynamicsInnerWithUpdate<BodyType, ParticlesType, MaterialType>
	{
	protected:
		virtual void Initialization(size_t index_particle_i, Real dt = 0.0) = 0;
		InnerFunctor functor_initialization_;
	public:
		ParticleDynamicsInner1Level(SPHBodyInnerRelation* body_inner_relation)
			: ParticleDynamicsInnerWithUpdate<BodyType, ParticlesType, MaterialType>(body_inner_relation),
			functor_initialization_(std::bind(&ParticleDynamicsInner1Level::Initialization, this, _1, _2)) {}
		virtual ~ParticleDynamicsInner1Level() {};

		virtual void exec(Real dt = 0.0);
		virtual void parallel_exec(Real dt = 0.0);
	};

	/**
	 * @class ParticleDynamicsContact
	 * @brief This is the class for contact interactions
	 */
	template <class BodyType, class ParticlesType, class MaterialType,
		class ContactBodyType, class ContactParticlesType, class ContactMaterialType = BaseMaterial>
		class ParticleDynamicsContact
		: public ParticleDynamicsWithContactConfigurations<BodyType, ParticlesType, MaterialType,
		ContactBodyType, ContactParticlesType, ContactMaterialType>
	{
	protected:
		virtual void ContactInteraction(size_t index_particle_i, Real dt = 0.0) = 0;
		InnerFunctor functor_contact_interaction_;
	public:
		explicit ParticleDynamicsContact(SPHBodyContactRelation* body_contact_relation)
			: ParticleDynamicsWithContactConfigurations<BodyType, ParticlesType, MaterialType,
			ContactBodyType, ContactParticlesType, ContactMaterialType>(body_contact_relation),
			functor_contact_interaction_(std::bind(&ParticleDynamicsContact::ContactInteraction, this, _1, _2)) {};
		virtual ~ParticleDynamicsContact() {};

		virtual void exec(Real dt = 0.0) override;
		virtual void parallel_exec(Real dt = 0.0) override;
	};

	/**
	* @class ParticleDynamicsComplex
	* @brief complex operations combining both inner and contact particle dynamics together
	*/
	template <class BodyType, class ParticlesType, class MaterialType,
		class ContactBodyType, class ContactParticlesType, class ContactMaterialType = BaseMaterial>
		class ParticleDynamicsComplex
		: public ParticleDynamicsWithComplexConfigurations<BodyType, ParticlesType, MaterialType,
		ContactBodyType, ContactParticlesType, ContactMaterialType>
	{
	protected:
		virtual void ComplexInteraction(size_t index_particle_i, Real dt = 0.0) = 0;
		InnerFunctor functor_complex_interaction_;

	public:
		ParticleDynamicsComplex(SPHBodyComplexRelation* body_complex_relation)
			: ParticleDynamicsWithComplexConfigurations<BodyType, ParticlesType, MaterialType,
			ContactBodyType, ContactParticlesType, ContactMaterialType>(body_complex_relation),
			functor_complex_interaction_(std::bind(&ParticleDynamicsComplex::ComplexInteraction, this, _1, _2)) {};
		virtual ~ParticleDynamicsComplex() {};

		virtual void exec(Real dt = 0.0) override;
		virtual void parallel_exec(Real dt = 0.0) override;
	};

	/**
	* @class ParticleDynamicsComplexWithUpdate
	* @brief complex operations with a update step
	*/
	template <class BodyType, class ParticlesType, class MaterialType,
		class ContactBodyType, class ContactParticlesType, class ContactMaterialType = BaseMaterial>
		class ParticleDynamicsComplexWithUpdate
		: public ParticleDynamicsComplex<BodyType, ParticlesType, MaterialType,
		ContactBodyType, ContactParticlesType, ContactMaterialType>
	{
	protected:
		virtual void Update(size_t index_particle_i, Real dt = 0.0) = 0;
		InnerFunctor functor_update_;

	public:
		ParticleDynamicsComplexWithUpdate(SPHBodyComplexRelation* body_complex_relation) 
			: ParticleDynamicsComplex<BodyType, ParticlesType, MaterialType,
			ContactBodyType, ContactParticlesType, ContactMaterialType>(body_complex_relation),
			functor_update_(std::bind(&ParticleDynamicsComplexWithUpdate::Update, this, _1, _2)) {};
		virtual ~ParticleDynamicsComplexWithUpdate() {};

		virtual void exec(Real dt = 0.0) override;
		virtual void parallel_exec(Real dt = 0.0) override;
	};

	/**
	* @class ParticleDynamicsComplex1Level
	* @brief complex operations with a initialization and a update step
	*/
	template <class BodyType, class ParticlesType, class MaterialType,
		class ContactBodyType, class ContactParticlesType, class ContactMaterialType = BaseMaterial>
		class ParticleDynamicsComplex1Level
		: public ParticleDynamicsComplexWithUpdate<BodyType, ParticlesType, MaterialType,
		ContactBodyType, ContactParticlesType, ContactMaterialType>
	{
	protected:
		virtual void Initialization(size_t index_particle_i, Real dt = 0.0) = 0;
		InnerFunctor functor_initialization_;

	public:
		ParticleDynamicsComplex1Level(SPHBodyComplexRelation* body_complex_relation)
			: ParticleDynamicsComplexWithUpdate<BodyType, ParticlesType, MaterialType,
			ContactBodyType, ContactParticlesType, ContactMaterialType>(body_complex_relation),
			functor_initialization_(std::bind(&ParticleDynamicsComplex1Level::Initialization, this, _1, _2)) {};
		virtual ~ParticleDynamicsComplex1Level() {};
	
		virtual void exec(Real dt = 0.0) override;
		virtual void parallel_exec(Real dt = 0.0) override;
	};

	/**
	* @class ParticleDynamicsComplexSplit
	* @brief complex split operations combining both inner and contact particle dynamics together
	*/
	template <class BodyType, class ParticlesType, class MaterialType,
		class ContactBodyType, class ContactParticlesType, class ContactMaterialType = BaseMaterial>
		class ParticleDynamicsComplexSplit
		: public ParticleDynamicsComplex1Level<BodyType, ParticlesType, MaterialType,
		ContactBodyType, ContactParticlesType, ContactMaterialType>
	{
	public:
		ParticleDynamicsComplexSplit(SPHBodyComplexRelation* body_complex_relation)
			: ParticleDynamicsComplex1Level<BodyType, ParticlesType, MaterialType,
			ContactBodyType, ContactParticlesType, ContactMaterialType>(body_complex_relation) {};
		virtual ~ParticleDynamicsComplexSplit() {};

		virtual void exec(Real dt = 0.0) override;
		virtual void parallel_exec(Real dt = 0.0) override;
	};

	/**
	  * @class ParticleDynamicsCellListSplitting
	  * @brief This is for using splitting algorithm for inner particle interactions
	  * which does not use particle configuration data for
	  * particle interaction
	  */
	template <class BodyType, class ParticlesType, class MaterialType = BaseMaterial>
	class ParticleDynamicsCellListSplitting
		: public ParticleDynamics<void, BodyType, ParticlesType, MaterialType>
	{
	protected:
		Kernel* kernel_;
		Real cutoff_radius_;

		virtual void CellListInteraction(CellList* cell_list, Real dt = 0.0) = 0;
		CellListFunctor functor_cell_list_;
	public:
		explicit ParticleDynamicsCellListSplitting(SPHBody* body);
		virtual ~ParticleDynamicsCellListSplitting() {};

		virtual void exec(Real dt = 0.0) override;
		virtual void parallel_exec(Real dt = 0.0) override;
	};

	/**
	 * @class ParticleDynamicsInnerSplitting
	 * @brief This is for the splitting algorithm
	 */
	template <class BodyType, class ParticlesType, class MaterialType = BaseMaterial>
	class ParticleDynamicsInnerSplitting
		: public ParticleDynamicsWithInnerConfigurations<BodyType, ParticlesType, MaterialType>
	{
	protected:
		virtual void InnerInteraction(size_t index_particle_i, Real dt = 0.0) = 0;
		InnerFunctor functor_inner_interaction_;
	public:
		explicit ParticleDynamicsInnerSplitting(SPHBodyInnerRelation* body_inner_relation)
			: ParticleDynamicsWithInnerConfigurations<BodyType, ParticlesType, MaterialType>(body_inner_relation),
			functor_inner_interaction_(std::bind(&ParticleDynamicsInnerSplitting::InnerInteraction, this, _1, _2)) {};
		virtual ~ParticleDynamicsInnerSplitting() {};

		virtual void exec(Real dt = 0.0) override;
		virtual void parallel_exec(Real dt = 0.0) override;
	};

	/**
	 * @class ParticleDynamicsComplexSplitting
	 * @brief This is for the splitting algorithm
	 * which taking account wall boundary conditions
	 */
	template <class BodyType, class ParticlesType, class MaterialType,
		class ContactBodyType, class ContactParticlesType, class ContactMaterialType = BaseMaterial>
		class ParticleDynamicsComplexSplitting
		: public ParticleDynamicsWithComplexConfigurations<BodyType, ParticlesType, MaterialType,
		ContactBodyType, ContactParticlesType, ContactMaterialType>
	{
	protected:
		/** the particle interaction also taking account wall boundary conditions,
		  * but the function form is the same as the inner interaction. */
		virtual void ParticleInteraction(size_t index_particle_i, Real dt = 0.0) = 0;
		InnerFunctor functor_particle_interaction_;
	public:
		explicit ParticleDynamicsComplexSplitting(SPHBodyComplexRelation* body_complex_relation)
			: ParticleDynamicsWithContactConfigurations<BodyType, ParticlesType, MaterialType,
			ContactBodyType, ContactParticlesType, ContactMaterialType>(body_complex_relation),
			functor_particle_interaction_(std::bind(&ParticleDynamicsComplexSplitting::ParticleInteraction, this, _1, _2)) {};
		virtual ~ParticleDynamicsComplexSplitting() {};

		virtual void exec(Real dt = 0.0) override;
		virtual void parallel_exec(Real dt = 0.0) override;
	};
}
