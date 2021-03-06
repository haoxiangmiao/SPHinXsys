/**
 * @file 	solid_dynamics.cpp
 * @author	Luhui Han, Chi ZHang and Xiangyu Hu
 * @version	0.1
 */

#include "solid_dynamics.h"

using namespace SimTK;
//=================================================================================================//
namespace SPH
{
	//=================================================================================================//
	namespace solid_dynamics
	{
		//=================================================================================================//
		void NormalDirectionSummation::ComplexInteraction(size_t index_particle_i, Real dt)
		{
			SolidParticleData &solid_data_i = particles_->solid_body_data_[index_particle_i];

			Vecd gradient(0.0);
			Neighborhood& inner_neighborhood = inner_configuration_[index_particle_i];
			CommonRelationList& inner_common_relations = inner_neighborhood.common_relation_list_;
			for (size_t n = 0; n != inner_neighborhood.current_size_; ++n)
			{
				gradient += inner_common_relations[n].dW_ij_ * inner_common_relations[n].e_ij_;
			}

			/** Contact interaction. */
			for (size_t k = 0; k < contact_configuration_.size(); ++k)
			{
				Neighborhood& contact_neighborhood = contact_configuration_[k][index_particle_i];
				CommonRelationList& contact_common_relations = contact_neighborhood.common_relation_list_;
				for (size_t n = 0; n != contact_neighborhood.current_size_; ++n)
				{
					gradient += contact_common_relations[n].dW_ij_ * contact_common_relations[n].e_ij_;
				}
			}

			solid_data_i.n_0_ = - gradient / (gradient.norm() + Eps);;
			solid_data_i.n_ = solid_data_i.n_0_;
		}
		//=================================================================================================//
		void NormalDirectionReNormalization::ComplexInteraction(size_t index_particle_i, Real dt)
		{
			BaseParticleData &base_particle_data_i = particles_->base_particle_data_[index_particle_i];
			SolidParticleData &solid_data_i = particles_->solid_body_data_[index_particle_i];

			Matd local_configuration(0.0);
			Vecd gradient(0.0);

			Neighborhood& inner_neighborhood = inner_configuration_[index_particle_i];
			CommonRelationList& inner_common_relations = inner_neighborhood.common_relation_list_;
			for (size_t n = 0; n != inner_neighborhood.current_size_; ++n)
			{
				CommonRelation& common_relation = inner_common_relations[n];
				size_t index_particle_j = common_relation.j_;
				BaseParticleData &base_particle_data_j = particles_->base_particle_data_[index_particle_j];

				Vecd gradw_ij = common_relation.dW_ij_ * common_relation.e_ij_;
				Vecd r_ij = -common_relation.r_ij_ * common_relation.e_ij_;
				local_configuration += base_particle_data_j.Vol_0_ * SimTK::outer(r_ij, gradw_ij);
				gradient += gradw_ij * base_particle_data_j.Vol_0_;
			}

			/** Contact interaction. */
			for (size_t k = 0; k < contact_configuration_.size(); ++k)
			{
				Neighborhood& contact_neighborhood = contact_configuration_[k][index_particle_i];
				CommonRelationList& contact_common_relations = contact_neighborhood.common_relation_list_;
				for (size_t n = 0; n != contact_neighborhood.current_size_; ++n)
				{
					CommonRelation& common_relation = contact_common_relations[n];
					size_t index_particle_j = common_relation.j_;
					BaseParticleData& base_particle_data_j
						= contact_particles_[k]->base_particle_data_[index_particle_j];

					Vecd gradw_ij = common_relation.dW_ij_ * common_relation.e_ij_;
					Vecd r_ij = -common_relation.r_ij_ * common_relation.e_ij_;
					local_configuration += base_particle_data_j.Vol_0_ * SimTK::outer(r_ij, gradw_ij);
					gradient += gradw_ij * base_particle_data_j.Vol_0_;
				}
			}

			Matd correction_matrix = inverse(local_configuration);
			Vecd n_temp_ = ~correction_matrix * gradient;
			if (n_temp_.norm() <= 0.75)	{
				solid_data_i.n_0_ = Vecd(0.0);
			}
			else {
				solid_data_i.n_0_ = -n_temp_ / (n_temp_.norm() + Eps);
			}
			solid_data_i.n_ = solid_data_i.n_0_;
		}
		//=================================================================================================//
		void InitializeDisplacement::Update(size_t index_particle_i, Real dt)
		{
			BaseParticleData &base_particle_data_i
				= particles_->base_particle_data_[index_particle_i];
			ElasticSolidParticleData &elastic_data_i
				= particles_->elastic_body_data_[index_particle_i];

			elastic_data_i.pos_temp_ = base_particle_data_i.pos_n_;
		}
		//=================================================================================================//
		void UpdateAverageVelocity::Update(size_t index_particle_i, Real dt)
		{
			BaseParticleData &base_particle_data_i = particles_->base_particle_data_[index_particle_i];
			SolidParticleData &solid_data_i = particles_->solid_body_data_[index_particle_i];
			ElasticSolidParticleData &elastic_data_i = particles_->elastic_body_data_[index_particle_i];

			solid_data_i.vel_ave_ = (base_particle_data_i.pos_n_ - elastic_data_i.pos_temp_) / (dt + TinyReal);
		}
		//=================================================================================================//
		void FluidViscousForceOnSolid::ContactInteraction(size_t index_particle_i, Real dt)
		{
			BaseParticleData& base_particle_data_i = particles_->base_particle_data_[index_particle_i];
			SolidParticleData &solid_data_i = particles_->solid_body_data_[index_particle_i];
			solid_data_i.viscous_force_from_fluid_ = Vecd(0);

			Vecd force(0);
			/** Contact interaction. */
			for (size_t k = 0; k < contact_configuration_.size(); ++k)
			{
				Neighborhood& contact_neighborhood = contact_configuration_[k][index_particle_i];
				CommonRelationList& contact_common_relations = contact_neighborhood.common_relation_list_;
				for (size_t n = 0; n != contact_neighborhood.current_size_; ++n)
				{
					CommonRelation& common_relation = contact_common_relations[n];
					size_t index_particle_j = common_relation.j_;
					BaseParticleData& base_particle_data_j
						= contact_particles_[k]->base_particle_data_[index_particle_j];
					FluidParticleData& fluid_data_j = contact_particles_[k]->fluid_particle_data_[index_particle_j];

					//froce due to viscousity
					//viscous force with a simple wall model for high-Reynolds number flow
					Vecd vel_detivative = 2.0 * (solid_data_i.vel_ave_ - base_particle_data_j.vel_n_)
						/ (common_relation.r_ij_ + 0.01 * smoothing_length_);
					Real vel_difference = 0.0 * (solid_data_i.vel_ave_ - base_particle_data_j.vel_n_).norm()
						* common_relation.r_ij_;

					force += 2.0 * SMAX(mu_, fluid_data_j.rho_n_ * vel_difference)
						* vel_detivative * base_particle_data_i.Vol_ * base_particle_data_j.Vol_
						* common_relation.dW_ij_;
				}
			}

			solid_data_i.viscous_force_from_fluid_ += force;
		}
		//=================================================================================================//
		void FluidAngularConservativeViscousForceOnSolid::ContactInteraction(size_t index_particle_i, Real dt)
		{
			BaseParticleData& base_particle_data_i = particles_->base_particle_data_[index_particle_i];
			SolidParticleData &solid_data_i = particles_->solid_body_data_[index_particle_i];
			solid_data_i.viscous_force_from_fluid_ = Vecd(0);

			Vecd force(0);
			/** Contact interaction. */
			for (size_t k = 0; k < contact_configuration_.size(); ++k)
			{
				Neighborhood& contact_neighborhood = contact_configuration_[k][index_particle_i];
				CommonRelationList& contact_common_relations = contact_neighborhood.common_relation_list_;
				for (size_t n = 0; n != contact_neighborhood.current_size_; ++n)
				{
					CommonRelation& common_relation = contact_common_relations[n];
					size_t index_particle_j = common_relation.j_;
					BaseParticleData& base_particle_data_j
						= contact_particles_[k]->base_particle_data_[index_particle_j];
					FluidParticleData& fluid_data_j = contact_particles_[k]->fluid_particle_data_[index_particle_j];

					/** The following viscous force is given in Monaghan 2005 (Rep. Prog. Phys.), it seems that
					 * is formulation is more accurate thant the previsou one for Taygree-Vortex flow. */
					Real v_r_ij = dot(solid_data_i.vel_ave_ - base_particle_data_j.vel_n_,
						common_relation.r_ij_ * common_relation.e_ij_);
					Real vel_difference = 0.0 * (solid_data_i.vel_ave_ - base_particle_data_j.vel_n_).norm()
						* common_relation.r_ij_;
					Real eta_ij = 8.0 * SMAX(mu_, fluid_data_j.rho_n_ * vel_difference) * v_r_ij /
						(common_relation.r_ij_ * common_relation.r_ij_ + 0.01 * smoothing_length_);
					force += eta_ij * base_particle_data_i.Vol_ * base_particle_data_j.Vol_
						* common_relation.dW_ij_ * common_relation.e_ij_;
				}
			}

			solid_data_i.viscous_force_from_fluid_ += force;
		}
		//=================================================================================================//
		TotalViscousForceOnSolid
			::TotalViscousForceOnSolid(SolidBody *body) : SolidDynamicsSum<Vecd>(body)
		{
			initial_reference_ = Vecd(0);
		}
		//=================================================================================================//
		Vecd TotalViscousForceOnSolid::ReduceFunction(size_t index_particle_i, Real dt)
		{
			SolidParticleData &solid_data_i = particles_->solid_body_data_[index_particle_i];

			return solid_data_i.viscous_force_from_fluid_;
		}
		//=================================================================================================//
		Vecd TotalForceOnSolid::ReduceFunction(size_t index_particle_i, Real dt)
		{
			SolidParticleData &solid_data_i = particles_->solid_body_data_[index_particle_i];

			return solid_data_i.force_from_fluid_;
		}
		//=================================================================================================//
		void FluidPressureForceOnSolid::ContactInteraction(size_t index_particle_i, Real dt)
		{
			BaseParticleData& base_particle_data_i = particles_->base_particle_data_[index_particle_i];
			SolidParticleData &solid_data_i = particles_->solid_body_data_[index_particle_i];
			solid_data_i.force_from_fluid_ = solid_data_i.viscous_force_from_fluid_;

			Vecd force(0);
			/** Contact interaction. */
			for (size_t k = 0; k < contact_configuration_.size(); ++k)
			{
				Neighborhood& contact_neighborhood = contact_configuration_[k][index_particle_i];
				CommonRelationList& contact_common_relations = contact_neighborhood.common_relation_list_;
				for (size_t n = 0; n != contact_neighborhood.current_size_; ++n)
				{
					CommonRelation& common_relation = contact_common_relations[n];
					size_t index_particle_j = common_relation.j_;
					BaseParticleData& base_particle_data_j
						= contact_particles_[k]->base_particle_data_[index_particle_j];
					FluidParticleData& fluid_data_j = contact_particles_[k]->fluid_particle_data_[index_particle_j];

					Vecd e_ij = common_relation.e_ij_;
					Real face_wall_external_acceleration
						= dot((base_particle_data_j.dvel_dt_others_ - solid_data_i.dvel_dt_ave_), e_ij);
					Real p_star = fluid_data_j.p_ + 0.5 * fluid_data_j.rho_n_
						* common_relation.r_ij_ * SMAX(0.0, face_wall_external_acceleration);

					//force due to pressure
					force -= 2.0 * p_star * e_ij 
								 * base_particle_data_i.Vol_ * base_particle_data_j.Vol_ * common_relation.dW_ij_;
				}
			}
			
			solid_data_i.force_from_fluid_ += force;
		}
		//=================================================================================================//
		GetAcousticTimeStepSize::GetAcousticTimeStepSize(SolidBody* body)
			: ElasticSolidDynamicsMinimum(body)
		{
			smoothing_length_ = body->kernel_->GetSmoothingLength();
			initial_reference_ = DBL_MAX;
		}
		//=================================================================================================//
		Real GetAcousticTimeStepSize::ReduceFunction(size_t index_particle_i, Real dt)
		{
			BaseParticleData &base_particle_data_i = particles_->base_particle_data_[index_particle_i];
			ElasticSolidParticleData &elastic_data_i = particles_->elastic_body_data_[index_particle_i];

			//since the particle does not change its configuration in pressure relaxation step
			//I chose a time-step size according to Eulerian method
			Real sound_speed = material_->getReferenceSoundSpeed();
			return 0.6 * SMIN(sqrt(smoothing_length_ / (base_particle_data_i.dvel_dt_.norm() + TinyReal)),
				smoothing_length_ / (sound_speed + base_particle_data_i.vel_n_.norm()));
		}
		//=================================================================================================//
		void CorrectConfiguration::InnerInteraction(size_t index_particle_i, Real dt)
		{
			SolidParticleData &solid_data_i = particles_->solid_body_data_[index_particle_i];

			/** a small number added to diagnal to avoid divide zero */
			Matd local_configuration(Eps);
			Neighborhood& inner_neighborhood = inner_configuration_[index_particle_i];
			CommonRelationList& inner_common_relations = inner_neighborhood.common_relation_list_;
			for (size_t n = 0; n != inner_neighborhood.current_size_; ++n)
			{
				CommonRelation& common_relation = inner_common_relations[n];
				size_t index_particle_j = common_relation.j_;
				BaseParticleData &base_particle_data_j = particles_->base_particle_data_[index_particle_j];

				Vecd gradw_ij = common_relation.dW_ij_ * common_relation.e_ij_;
				Vecd r_ji = - common_relation.r_ij_ * common_relation.e_ij_;
				local_configuration += base_particle_data_j.Vol_0_ * SimTK::outer(r_ji, gradw_ij);
			}
			/** note that I have changed to use stadrad linear solver here*/
			solid_data_i.B_ = SimTK::inverse(local_configuration);
		}
		//=================================================================================================//
		void DeformationGradientTensorBySummation::InnerInteraction(size_t index_particle_i, Real dt)
		{
			BaseParticleData &base_particle_data_i = particles_->base_particle_data_[index_particle_i];
			SolidParticleData &solid_data_i = particles_->solid_body_data_[index_particle_i];
			ElasticSolidParticleData &elastic_data_i = particles_->elastic_body_data_[index_particle_i];

			Matd deformation(0.0);
			Neighborhood& inner_neighborhood = inner_configuration_[index_particle_i];
			CommonRelationList& inner_common_relations = inner_neighborhood.common_relation_list_;
			for (size_t n = 0; n != inner_neighborhood.current_size_; ++n)
			{
				CommonRelation& common_relation = inner_common_relations[n];
				size_t index_particle_j = common_relation.j_;
				BaseParticleData &base_particle_data_j = particles_->base_particle_data_[index_particle_j];

				Vecd gradw_ij = common_relation.dW_ij_ * common_relation.e_ij_;
				deformation -= base_particle_data_j.Vol_0_
					*SimTK::outer((base_particle_data_i.pos_n_ - base_particle_data_j.pos_n_), gradw_ij);
			}

			elastic_data_i.F_ = deformation * solid_data_i.B_;
		}
		//=================================================================================================//
		void StressRelaxationFirstHalf::Initialization(size_t index_particle_i, Real dt)
		{
			BaseParticleData &base_particle_data_i = particles_->base_particle_data_[index_particle_i];
			SolidParticleData &solid_data_i = particles_->solid_body_data_[index_particle_i];
			ElasticSolidParticleData &elastic_data_i = particles_->elastic_body_data_[index_particle_i];

			elastic_data_i.F_ += elastic_data_i.dF_dt_*dt * 0.5;
			solid_data_i.rho_n_ = solid_data_i.rho_0_ / det(elastic_data_i.F_);
			base_particle_data_i.Vol_ = solid_data_i.mass_ / solid_data_i.rho_n_;
			elastic_data_i.stress_ = material_->ConstitutiveRelation(elastic_data_i.F_, index_particle_i)
				+ material_->NumericalDampingStress(elastic_data_i.F_, elastic_data_i.dF_dt_, numerical_viscosity_, index_particle_i);
			base_particle_data_i.pos_n_ += base_particle_data_i.vel_n_*dt * 0.5;

		}
		//=================================================================================================//
		void StressRelaxationFirstHalf::InnerInteraction(size_t index_particle_i, Real dt)
		{
			BaseParticleData &base_particle_data_i = particles_->base_particle_data_[index_particle_i];
			SolidParticleData &solid_data_i = particles_->solid_body_data_[index_particle_i];
			ElasticSolidParticleData &elastic_data_i = particles_->elastic_body_data_[index_particle_i];

			//including gravity and force from fluid
			Vecd acceleration = base_particle_data_i.dvel_dt_others_ 
				+ solid_data_i.force_from_fluid_/ solid_data_i.mass_;

			Neighborhood& inner_neighborhood = inner_configuration_[index_particle_i];
			CommonRelationList& inner_common_relations = inner_neighborhood.common_relation_list_;
			for (size_t n = 0; n != inner_neighborhood.current_size_; ++n)
			{
				CommonRelation& common_relation = inner_common_relations[n];
				size_t index_particle_j = common_relation.j_;
				BaseParticleData &base_particle_data_j = particles_->base_particle_data_[index_particle_j];
				SolidParticleData &solid_data_j = particles_->solid_body_data_[index_particle_j];
				ElasticSolidParticleData &elastic_data_j = particles_->elastic_body_data_[index_particle_j];

				acceleration += (elastic_data_i.stress_ *solid_data_i.B_
					+ elastic_data_j.stress_*solid_data_j.B_)
					* common_relation.dW_ij_ * common_relation.e_ij_
					* base_particle_data_j.Vol_0_ / solid_data_i.rho_0_;
			}
			base_particle_data_i.dvel_dt_ = acceleration;
		}
		//=================================================================================================//
		void StressRelaxationFirstHalf::Update(size_t index_particle_i, Real dt)
		{
			BaseParticleData &base_particle_data_i = particles_->base_particle_data_[index_particle_i];
			ElasticSolidParticleData &elastic_data_i = particles_->elastic_body_data_[index_particle_i];

			base_particle_data_i.vel_n_ += base_particle_data_i.dvel_dt_* dt;
		}
		//=================================================================================================//
		void StressRelaxationSecondHalf::Initialization(size_t index_particle_i, Real dt)
		{
			BaseParticleData &base_particle_data_i 	= particles_->base_particle_data_[index_particle_i];
			ElasticSolidParticleData &elastic_data_i = particles_->elastic_body_data_[index_particle_i];

			base_particle_data_i.pos_n_ += base_particle_data_i.vel_n_ * dt * 0.5;
		}
		//=================================================================================================//
		void StressRelaxationSecondHalf::InnerInteraction(size_t index_particle_i, Real dt)
		{
			BaseParticleData &base_particle_data_i = particles_->base_particle_data_[index_particle_i];
			SolidParticleData &solid_data_i = particles_->solid_body_data_[index_particle_i];
			ElasticSolidParticleData &elastic_data_i = particles_->elastic_body_data_[index_particle_i];

			Matd deformation_gradient_change_rate(0);
			Neighborhood& inner_neighborhood = inner_configuration_[index_particle_i];
			CommonRelationList& inner_common_relations = inner_neighborhood.common_relation_list_;
			for (size_t n = 0; n != inner_neighborhood.current_size_; ++n)
			{
				CommonRelation& common_relation = inner_common_relations[n];
				size_t index_particle_j = common_relation.j_;
				BaseParticleData &base_particle_data_j = particles_->base_particle_data_[index_particle_j];
				SolidParticleData &solid_data_j = particles_->solid_body_data_[index_particle_j];
				ElasticSolidParticleData &elastic_data_j = particles_->elastic_body_data_[index_particle_j];

				Vecd gradw_ij = common_relation.dW_ij_ * common_relation.e_ij_;
				deformation_gradient_change_rate
					-= base_particle_data_j.Vol_0_
					*SimTK::outer((base_particle_data_i.vel_n_ - base_particle_data_j.vel_n_), gradw_ij);
			}
			elastic_data_i.dF_dt_ = deformation_gradient_change_rate* solid_data_i.B_;
		}
		//=================================================================================================//
		void StressRelaxationSecondHalf::Update(size_t index_particle_i, Real dt)
		{
			ElasticSolidParticleData &elastic_data_i = particles_->elastic_body_data_[index_particle_i];

			elastic_data_i.F_ += elastic_data_i.dF_dt_ * dt * 0.5;
		}
		//=================================================================================================//
		void ConstrainSolidBodyRegion::Update(size_t index_particle_i, Real dt)
		{
			BaseParticleData &base_particle_data_i
				= particles_->base_particle_data_[index_particle_i];
			SolidParticleData &solid_data_i
				= particles_->solid_body_data_[index_particle_i];

			Vecd pos_old = base_particle_data_i.pos_n_;
			base_particle_data_i.pos_n_ = base_particle_data_i.pos_0_ + GetDisplacement(pos_old);
			base_particle_data_i.vel_n_ = GetVelocity(pos_old);
			base_particle_data_i.dvel_dt_ = GetAcceleration(pos_old);
			/** the average values are prescirbed also. */
			solid_data_i.vel_ave_ = base_particle_data_i.vel_n_;
			solid_data_i.dvel_dt_ave_ = base_particle_data_i.dvel_dt_;
		}
		//=================================================================================================//
		void ConstrainSolidBodyRegionSinusoidalMotion::Update(size_t index_particle_i, Real dt)
		{
			BaseParticleData &base_particle_data_i
				= particles_->base_particle_data_[index_particle_i];
			SolidParticleData &solid_data_i
				= particles_->solid_body_data_[index_particle_i];

			Vecd pos_old = base_particle_data_i.pos_n_;
			base_particle_data_i.pos_n_ = base_particle_data_i.pos_0_ + GetDisplacement(pos_old);
			base_particle_data_i.vel_n_ = GetVelocity(pos_old);
			base_particle_data_i.dvel_dt_ = GetAcceleration(pos_old);
			/** the average values are prescirbed also. */
			solid_data_i.vel_ave_ = base_particle_data_i.vel_n_;
			solid_data_i.dvel_dt_ave_ = base_particle_data_i.dvel_dt_;
		}
		//=================================================================================================//
		Vecd ConstrainSolidBodyRegionSinusoidalMotion::GetDisplacement(Vecd &pos)
		{	
			Vecd disp(0.0);
			disp[1] = h_m_ * sin(2.0 * Pi * f_ * GlobalStaticVariables::physical_time_ + Real((id_ -1)) * phi_);
			return disp;
		}
		//=================================================================================================//
		Vecd ConstrainSolidBodyRegionSinusoidalMotion::GetVelocity(Vecd &pos)
		{
			Vecd disp(0.0);
			disp[1] = h_m_ * 2.0 * Pi * f_ * cos(2.0 * Pi * f_ * GlobalStaticVariables::physical_time_ + Real((id_ -1)) * phi_);
			return disp;
		}
		//=================================================================================================//
		Vecd ConstrainSolidBodyRegionSinusoidalMotion::GetAcceleration(Vecd &pos)
		{
			Vecd disp(0.0);
			disp[1] = -h_m_ * 2.0 * Pi * f_ * 2.0 * Pi * f_ * cos(2.0 * Pi * f_ * GlobalStaticVariables::physical_time_ + Real((id_ -1)) * phi_);
			return disp;
		}
		//=================================================================================================//
		void constrainNormDirichletBoundary::Update(size_t index_particle_i, Real dt)
		{
			BaseParticleData &base_particle_data_i
				= particles_->base_particle_data_[index_particle_i];
			SolidParticleData &solid_data_i
				= particles_->solid_body_data_[index_particle_i];

			Vecd pos_old = base_particle_data_i.pos_n_;
			base_particle_data_i.pos_n_[axis_id_] = base_particle_data_i.pos_0_[axis_id_];
			base_particle_data_i.vel_n_[axis_id_] = 0.0;
			base_particle_data_i.dvel_dt_[axis_id_] = 0.0;
			/** the average values are prescirbed also. */
			solid_data_i.vel_ave_ = base_particle_data_i.vel_n_;
			solid_data_i.dvel_dt_ave_ = base_particle_data_i.dvel_dt_;
		}
		//=================================================================================================//
		void ImposeExternalForce::Update(size_t index_particle_i, Real dt)
		{
			BaseParticleData &base_particle_data_i = particles_->base_particle_data_[index_particle_i];
			SolidParticleData &solid_data_i = particles_->solid_body_data_[index_particle_i];

			Vecd induced_acceleration = GetAcceleration(base_particle_data_i.pos_0_);
			base_particle_data_i.vel_n_ += induced_acceleration * dt;
			solid_data_i.vel_ave_ = base_particle_data_i.vel_n_;
		}
		//=================================================================================================//
		ConstrainSolidBodyPartBySimBody::ConstrainSolidBodyPartBySimBody(SolidBody *body,
				SolidBodyPartForSimbody *body_part,
				SimTK::MultibodySystem &MBsystem,
				SimTK::MobilizedBody &mobod,
				SimTK::Force::DiscreteForces &force_on_bodies,
				SimTK::RungeKuttaMersonIntegrator &integ)
			: PartDynamicsByParticle<SolidBody, SolidParticles, SolidBodyPartForSimbody>(body, body_part),
			MBsystem_(MBsystem), mobod_(mobod), force_on_bodies_(force_on_bodies), integ_(integ)
		{
			simbody_state_ = &integ_.getState();
			MBsystem_.realize(*simbody_state_, Stage::Acceleration);
			initial_mobod_origin_location_ = mobod_.getBodyOriginLocation(*simbody_state_);
		}
		//=================================================================================================//
		void  ConstrainSolidBodyPartBySimBody::setupDynamics(Real dt)
		{
			body_->setNewlyUpdated();
			simbody_state_ = &integ_.getState();
			MBsystem_.realize(*simbody_state_, Stage::Acceleration);
		}
		//=================================================================================================//
		ForceOnSolidBodyPartForSimBody
			::ForceOnSolidBodyPartForSimBody(SolidBody *body,
				SolidBodyPartForSimbody *body_part,
				SimTK::MultibodySystem &MBsystem,
				SimTK::MobilizedBody &mobod,
				SimTK::Force::DiscreteForces &force_on_bodies,
				SimTK::RungeKuttaMersonIntegrator &integ)
			: SolidDynamicsConstraintForSimbodySum<SpatialVec>(body, body_part),
			MBsystem_(MBsystem), mobod_(mobod), force_on_bodies_(force_on_bodies), integ_(integ)
		{
			initial_reference_ = SpatialVec(Vec3(0), Vec3(0));
		}
		//=================================================================================================//
		void ForceOnSolidBodyPartForSimBody::SetupReduce()
		{
			simbody_state_ = &integ_.getState();
			MBsystem_.realize(*simbody_state_, Stage::Acceleration);
			current_mobod_origin_location_ = mobod_.getBodyOriginLocation(*simbody_state_);
		}
		//=================================================================================================//
		ForceOnElasticBodyPartForSimBody
			::ForceOnElasticBodyPartForSimBody(SolidBody *body,
				SolidBodyPartForSimbody *body_part,
				SimTK::MultibodySystem &MBsystem,
				SimTK::MobilizedBody &mobod,
				SimTK::Force::DiscreteForces &force_on_bodies,
				SimTK::RungeKuttaMersonIntegrator &integ)
			: ElasticSolidDynamicsConstraintForSimbodySum<SpatialVec>(body, body_part),
			MBsystem_(MBsystem), mobod_(mobod), force_on_bodies_(force_on_bodies), integ_(integ)
		{
			initial_reference_ = SpatialVec(Vec3(0), Vec3(0));
		}
		//=================================================================================================//
		void ForceOnElasticBodyPartForSimBody::SetupReduce()
		{
			simbody_state_ = &integ_.getState();
			MBsystem_.realize(*simbody_state_, Stage::Acceleration);
			current_mobod_origin_location_ = mobod_.getBodyOriginLocation(*simbody_state_);
		}
		//=================================================================================================//
		DampingBySplittingAlgorithm
			::DampingBySplittingAlgorithm(SPHBodyInnerRelation* body_inner_relation)
			: ElasticSolidDynamicsInnerSplitting(body_inner_relation), 
			eta_(material_->getPhysicalViscosity())
		{

		}
		//=================================================================================================//
		void  DampingBySplittingAlgorithm::setupDynamics(Real dt)
		{
			body_->setNewlyUpdated();
		}
		//=================================================================================================//
		void DampingBySplittingAlgorithm
			::InnerInteraction(size_t index_particle_i, Real dt)
		{

			BaseParticleData &base_particle_data_i = particles_->base_particle_data_[index_particle_i];
			SolidParticleData &solid_data_i = particles_->solid_body_data_[index_particle_i];
			ElasticSolidParticleData &elastic_data_i = particles_->elastic_body_data_[index_particle_i];
			Real mass_i = solid_data_i.mass_;

			Vecd error(0);
			Real parameter_a(0);
			Real parameter_c(0);
			Neighborhood& inner_neighborhood = inner_configuration_[index_particle_i];
			CommonRelationList& inner_common_relations = inner_neighborhood.common_relation_list_;
			for (size_t n = 0; n != inner_neighborhood.current_size_; ++n)
			{
				CommonRelation& common_relation = inner_common_relations[n];
				size_t index_particle_j = common_relation.j_;
				BaseParticleData &base_particle_data_j
					= particles_->base_particle_data_[index_particle_j];
				SolidParticleData& solid_body_data_j
					= particles_->solid_body_data_[index_particle_j];
				Real mass_j = solid_body_data_j.mass_;

				//linear projection 
				Vecd vel_detivative = (base_particle_data_i.vel_n_ - base_particle_data_j.vel_n_);
				Real parameter_b = 2.0 * eta_ * common_relation.dW_ij_
					* base_particle_data_i.Vol_0_ * base_particle_data_j.Vol_0_ * dt
					/ common_relation.r_ij_;

				error -= vel_detivative * parameter_b;
				parameter_a += parameter_b;
				parameter_c += parameter_b * parameter_b;
			}
			
			parameter_a -= mass_i;
			Real parameter_l = parameter_a * parameter_a + parameter_c;
			Vecd parameter_k = error / (parameter_l + TinyReal);
			base_particle_data_i.vel_n_ += parameter_k * parameter_a;

			for (size_t n = 0; n != inner_neighborhood.current_size_; ++n)
			{
				CommonRelation& common_relation = inner_common_relations[n];
				size_t index_particle_j = common_relation.j_;
				BaseParticleData &base_particle_data_j
					= particles_->base_particle_data_[index_particle_j];
				SolidParticleData& solid_body_data_j
					= particles_->solid_body_data_[index_particle_j];
				Real mass_j = solid_body_data_j.mass_;

				Real parameter_b = 2.0 * eta_ * common_relation.dW_ij_
					* base_particle_data_i.Vol_0_ * base_particle_data_j.Vol_0_ * dt
					/ common_relation.r_ij_;

				//predicted velocity at particle j
				Vecd vel_j = base_particle_data_j.vel_n_ - parameter_k * parameter_b;
				Vecd vel_detivative = (base_particle_data_i.vel_n_ - vel_j);

				//viscous force in conservation form
				base_particle_data_j.vel_n_ -= vel_detivative * parameter_b / mass_j;
			}
		}
		//=================================================================================================//
		DampingBySplittingPairwise
			::DampingBySplittingPairwise(SPHBodyInnerRelation* body_inner_relation)
			: ElasticSolidDynamicsInnerSplitting(body_inner_relation), eta_(material_->getPhysicalViscosity())
		{

		}
		//=================================================================================================//
		void DampingBySplittingPairwise
			::InnerInteraction(size_t index_particle_i, Real dt)
		{

			BaseParticleData& base_particle_data_i = particles_->base_particle_data_[index_particle_i];
			SolidParticleData& solid_data_i = particles_->solid_body_data_[index_particle_i];
			Real mass_i = solid_data_i.mass_;

			StdVec<Real> parameter_b(50);
			Neighborhood& inner_neighborhood = inner_configuration_[index_particle_i];
			CommonRelationList& inner_common_relations = inner_neighborhood.common_relation_list_;
			size_t number_of_neighbors = inner_neighborhood.current_size_;
			parameter_b.resize(number_of_neighbors);
			//forward sweep
			for (size_t n = 0; n != number_of_neighbors; ++n)
			{
				CommonRelation& common_relation = inner_common_relations[n];
				size_t index_particle_j = common_relation.j_;
				BaseParticleData& base_particle_data_j
					= particles_->base_particle_data_[index_particle_j];
				SolidParticleData& solid_data_j
					= particles_->solid_body_data_[index_particle_j];
				Real mass_j = solid_data_j.mass_;

				Vecd vel_detivative = (base_particle_data_i.vel_n_ - base_particle_data_j.vel_n_);
				parameter_b[n] = eta_ * common_relation.dW_ij_
					* base_particle_data_i.Vol_0_ * base_particle_data_j.Vol_0_ * dt
					/ common_relation.r_ij_;

				Vecd increment = parameter_b[n] * vel_detivative
					/ (mass_i * mass_j - parameter_b[n] * (mass_i + mass_j));
				base_particle_data_i.vel_n_ += increment * mass_j;
				base_particle_data_j.vel_n_ -= increment * mass_i;
			}

			//backward sweep
			for (size_t n = number_of_neighbors; n != 0; --n)
			{
				CommonRelation& common_relation = inner_common_relations[n - 1];
				size_t index_particle_j = common_relation.j_;
				BaseParticleData& base_particle_data_j
					= particles_->base_particle_data_[index_particle_j];
				SolidParticleData& solid_data_j
					= particles_->solid_body_data_[index_particle_j];
				Real mass_j = solid_data_j.mass_;

				Vecd vel_detivative = (base_particle_data_i.vel_n_ - base_particle_data_j.vel_n_);
				Vecd increment = parameter_b[n - 1] * vel_detivative
					/ (mass_i * mass_j - parameter_b[n - 1] * (mass_i + mass_j));

				base_particle_data_i.vel_n_ += increment * mass_j;
				base_particle_data_j.vel_n_ -= increment * mass_i;
			}
		}
		//=================================================================================================//
		DampingBySplittingWithRandomChoice
			::DampingBySplittingWithRandomChoice(SPHBodyInnerRelation* body_inner_relation, Real random_ratio)
			: DampingBySplittingAlgorithm(body_inner_relation), random_ratio_(random_ratio)
		{
			eta_ = eta_ / random_ratio_;
		}
		//=================================================================================================//
		bool DampingBySplittingWithRandomChoice::RandomChoice()
		{
			return ((double)rand() / (RAND_MAX)) < random_ratio_ ? true : false;
		}
		//=================================================================================================//
		void DampingBySplittingWithRandomChoice::exec(Real dt)
		{
			if (RandomChoice()) DampingBySplittingAlgorithm::exec(dt);
		}
		//=================================================================================================//
		//=================================================================================================//
		void DampingBySplittingWithRandomChoice::parallel_exec(Real dt)
		{
			if (RandomChoice()) DampingBySplittingAlgorithm::parallel_exec(dt);
		}
		//=================================================================================================//
	}
	//=================================================================================================//
}
//=================================================================================================//
