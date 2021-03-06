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
 * @file 	eletro_physiology.h
 * @brief 	In is file, we declaim the dynamics relavant to electrophysiology,
 * including diffusion, reaction and muscle activation. 
 * @author 	Chi Zhang and Xiangyu Hu
 * @version 0.2.1
 * 			From here, I will denote version a beta, e.g. 0.2.1, other than 0.1 as
 * 			we will introduce cardiac electrophysiology and cardaic mechanics herein.
 * 			Chi Zhang
 */
#pragma once

#include "all_particle_dynamics.h"
#include "diffusion_reaction_particles.h"
#include "particle_dynamics_diffusion_reaction.h"
#include "diffusion_reaction.h"
#include "elastic_solid.h"
#include "base_kernel.h"
#include "solid_body.h"
#include "solid_particles.h"

namespace SPH
{
	namespace electro_physiology
	{
		typedef DiffusionBase<SolidBody, SolidParticles, Solid> ElectroPhysiologyBase;
		typedef DiffusionReactionSimple<SolidBody, SolidParticles, Solid> ElectroPhysiologySimple;
		typedef DiffusionInner<SolidBody, SolidParticles, Solid> ElectroPhysiologyInner;
		/**
		 * @class ElectroPhysiologyInitialCondition
		 * @brief  set initial condition for a muscle body
		 * This is a abstract class to be override for case specific initial conditions.
		 */
		class ElectroPhysiologyInitialCondition : public ElectroPhysiologySimple
		{
		public:
			ElectroPhysiologyInitialCondition(SolidBody *body)
				: ElectroPhysiologySimple(body) {};
			virtual ~ElectroPhysiologyInitialCondition() {};
		};
        /**
		* @class GetElectroPhysiologyTimeStepSize
		* @brief Computing the acoustic time step size
		* computing time step size
		*/
		class GetElectroPhysiologyTimeStepSize 
			: public GetDiffusionTimeStepSize<SolidBody, SolidParticles, Solid>
		{
		public:
			explicit GetElectroPhysiologyTimeStepSize(SolidBody* body)
				: GetDiffusionTimeStepSize<SolidBody, SolidParticles, Solid>(body) {};
			virtual ~GetElectroPhysiologyTimeStepSize() {};
		};
        /**
		* @class ElectroPhysiologyDiffusionRelaxation
		* @brief Compute the diffusion relaxation process
		*/
		class ElectroPhysiologyDiffusionRelaxation : 
			public RelaxationOfAllDiffusionSpeciesRK2<SolidBody, SolidParticles, Solid>
		{
		public:
			ElectroPhysiologyDiffusionRelaxation(SPHBodyInnerRelation* body_inner_relation)
				: RelaxationOfAllDiffusionSpeciesRK2<SolidBody, SolidParticles, Solid>(body_inner_relation) {};
			virtual ~ElectroPhysiologyDiffusionRelaxation() {};
		};
        /**
		 * @class ElectroPhysiologyReactionRelaxationForward
		 * @brief Solv the reaction ODE equation of trans-membrane potential
		*/
		class ElectroPhysiologyReactionRelaxationForward
			: public RelaxationOfAllReactionsFoward<SolidBody, SolidParticles, Solid>
		{
		public:
			ElectroPhysiologyReactionRelaxationForward(SolidBody* body)
				: RelaxationOfAllReactionsFoward<SolidBody, SolidParticles, Solid>(body) {};
			virtual ~ElectroPhysiologyReactionRelaxationForward() {};
		};
		/**
		 * @class ElectroPhysiologyReactionRelaxationForward
		 * @brief Solv the reaction ODE equation of trans-membrane potential
		*/
		class ElectroPhysiologyReactionRelaxationBackward
			: public RelaxationOfAllReactionsBackward<SolidBody, SolidParticles, Solid>
		{
		public:
			ElectroPhysiologyReactionRelaxationBackward(SolidBody* body)
				: RelaxationOfAllReactionsBackward<SolidBody, SolidParticles, Solid>(body) {};
			virtual ~ElectroPhysiologyReactionRelaxationBackward() {};
		};
		/**
		 * @class ApplyStimulusCurrents
		 * @brief Apply specific stimulus currents
		*/
		class ApplyStimulusCurrents : public ElectroPhysiologySimple
		{
		protected:
			virtual void Update(size_t index_particle_i, Real dt = 0.0) override {};
		public:
			ApplyStimulusCurrents(SolidBody *body) : ElectroPhysiologySimple(body) {}
			virtual ~ApplyStimulusCurrents() {};
		};
    }
}
