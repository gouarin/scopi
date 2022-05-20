#pragma once

#include <plog/Log.h>
#include "plog/Initializers/RollingFileInitializer.h"
#include <vector>
#include <xtensor/xtensor.hpp>

#include "../container.hpp"
#include "../quaternion.hpp"
#include "../objects/neighbor.hpp"
#include "../utils.hpp"
#include "ViscousBase.hpp"

namespace scopi
{
    template<std::size_t dim>
    class ViscousWithFriction: public ViscousBase<ViscousWithFriction<dim>, dim>
    {
    public:
        using base_type = ViscousBase<ViscousWithFriction, dim>;

        ViscousWithFriction(std::size_t nparts, double dt);

        void create_matrix_constraint_coo_impl(const scopi_container<dim>& particles,
                                               const std::vector<neighbor<dim>>& contacts,
                                               std::size_t firstCol);
        void update_gamma_impl(const std::vector<neighbor<dim>>& contacts, xt::xtensor<double, 1> lambda);
        std::size_t number_row_matrix_impl(const std::vector<neighbor<dim>>& contacts);
        void create_vector_distances_impl(const std::vector<neighbor<dim>>& contacts);

        std::size_t get_nb_gamma_min_impl();

    protected:
        void set_gamma(const std::vector<neighbor<dim>>& contacts_new);

        std::size_t m_nb_gamma_min;
        double m_gamma_min;
        double m_mu;
    };

    template<std::size_t dim>
    void ViscousWithFriction<dim>::create_matrix_constraint_coo_impl(const scopi_container<dim>& particles,
                                                                     const std::vector<neighbor<dim>>& contacts,
                                                                     std::size_t firstCol)
    {
        std::size_t active_offset = particles.nb_inactive();
        std::size_t size = 6 * this->number_row_matrix(contacts);
        this->m_A_rows.resize(size);
        this->m_A_cols.resize(size);
        this->m_A_values.resize(size);

        std::size_t ic = 0;
        std::size_t index = 0;
        for (auto &c: contacts)
        {
            if (this->m_gamma[ic] != m_gamma_min)
            {
                if (c.i >= active_offset)
                {
                    for (std::size_t d = 0; d < 3; ++d)
                    {
                        this->m_A_rows[index] = ic;
                        this->m_A_cols[index] = firstCol + (c.i - active_offset)*3 + d;
                        this->m_A_values[index] = -this->m_dt*c.nij[d];
                        index++;
                        if (this->m_gamma[ic] < -this->m_tol)
                        {
                            this->m_A_rows[index] = contacts.size() - m_nb_gamma_min + ic;
                            this->m_A_cols[index] = firstCol + (c.i - active_offset)*3 + d;
                            this->m_A_values[index] = this->m_dt*c.nij[d];
                            index++;
                        }
                    }
                }

                if (c.j >= active_offset)
                {
                    for (std::size_t d = 0; d < 3; ++d)
                    {
                        this->m_A_rows[index] = ic;
                        this->m_A_cols[index] = firstCol + (c.j - active_offset)*3 + d;
                        this->m_A_values[index] = this->m_dt*c.nij[d];
                        index++;
                        if (this->m_gamma[ic] < -this->m_tol)
                        {
                            this->m_A_rows[index] = contacts.size() - m_nb_gamma_min + ic;
                            this->m_A_cols[index] = firstCol + (c.j - active_offset)*3 + d;
                            this->m_A_values[index] = -this->m_dt*c.nij[d];
                            index++;
                        }
                    }
                }

                auto ri_cross = cross_product<dim>(c.pi - particles.pos()(c.i));
                auto rj_cross = cross_product<dim>(c.pj - particles.pos()(c.j));
                auto Ri = rotation_matrix<3>(particles.q()(c.i));
                auto Rj = rotation_matrix<3>(particles.q()(c.j));

                if (c.i >= active_offset)
                {
                    std::size_t ind_part = c.i - active_offset;
                    auto dot = xt::eval(xt::linalg::dot(ri_cross, Ri));
                    for (std::size_t ip = 0; ip < 3; ++ip)
                    {
                        this->m_A_rows[index] = ic;
                        this->m_A_cols[index] = firstCol + 3*particles.nb_active() + 3*ind_part + ip;
                        this->m_A_values[index] = this->m_dt*(c.nij[0]*dot(0, ip)+c.nij[1]*dot(1, ip)+c.nij[2]*dot(2, ip));
                        index++;
                        if (this->m_gamma[ic] < -this->m_tol)
                        {
                            this->m_A_rows[index] = contacts.size() - m_nb_gamma_min + ic;
                            this->m_A_cols[index] = firstCol + 3*particles.nb_active() + 3*ind_part + ip;
                            this->m_A_values[index] = -this->m_dt*(c.nij[0]*dot(0, ip)+c.nij[1]*dot(1, ip)+c.nij[2]*dot(2, ip));
                            index++;
                        }
                    }
                }

                if (c.j >= active_offset)
                {
                    std::size_t ind_part = c.j - active_offset;
                    auto dot = xt::eval(xt::linalg::dot(rj_cross, Rj));
                    for (std::size_t ip = 0; ip < 3; ++ip)
                    {
                        this->m_A_rows[index] = ic;
                        this->m_A_cols[index] = firstCol + 3*particles.nb_active() + 3*ind_part + ip;
                        this->m_A_values[index] = -this->m_dt*(c.nij[0]*dot(0, ip)+c.nij[1]*dot(1, ip)+c.nij[2]*dot(2, ip));
                        index++;
                        if (this->m_gamma[ic] < -this->m_tol)
                        {
                            this->m_A_rows[index] = contacts.size() - m_nb_gamma_min + ic;
                            this->m_A_cols[index] = firstCol + 3*particles.nb_active() + 3*ind_part + ip;
                            this->m_A_values[index] = this->m_dt*(c.nij[0]*dot(0, ip)+c.nij[1]*dot(1, ip)+c.nij[2]*dot(2, ip));
                            index++;
                        }
                    }
                }

            }
            else
            {
                if (c.i >= active_offset)
                {
                    for (std::size_t d = 0; d < 3; ++d)
                    {
                        this->m_A_rows[index] = contacts.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 4*ic;
                        this->m_A_cols[index] = firstCol + (c.i - active_offset)*3 + d;
                        this->m_A_values[index] = -this->m_dt*c.nij[d];
                        index++;

                        this->m_A_rows[index] = contacts.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 4*m_nb_gamma_min + 4*ic;
                        this->m_A_cols[index] = firstCol + (c.i - active_offset)*3 + d;
                        this->m_A_values[index] = this->m_dt*c.nij[d];
                        index++;
                    }
                    for (std::size_t ind_row = 0; ind_row < 3; ++ind_row)
                    {
                        for (std::size_t ind_col = 0; ind_col < 3; ++ind_col)
                        {
                            this->m_A_rows[index] = contacts.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 4*ic + 1 + ind_row;
                            this->m_A_cols[index] = firstCol + (c.i - active_offset)*3 + ind_col;
                            this->m_A_values[index] = -this->m_dt*m_mu*c.nij[ind_row]*c.nij[ind_col];
                            if(ind_row == ind_col)
                            {
                                this->m_A_values[index] += this->m_dt*m_mu;
                            }
                            index++;
                        }

                        for (std::size_t ind_col = 0; ind_col < 3; ++ind_col)
                        {
                            this->m_A_rows[index] = contacts.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 4*m_nb_gamma_min + 4*ic + 1 + ind_row;
                            this->m_A_cols[index] = firstCol + (c.i - active_offset)*3 + ind_col;
                            this->m_A_values[index] = -this->m_dt*m_mu*c.nij[ind_row]*c.nij[ind_col];
                            if(ind_row == ind_col)
                            {
                                this->m_A_values[index] += this->m_dt*m_mu;
                            }
                            index++;
                        }
                    }
                }

                if (c.j >= active_offset)
                {
                    for (std::size_t d = 0; d < 3; ++d)
                    {
                        this->m_A_rows[index] = contacts.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 4*ic;
                        this->m_A_cols[index] = firstCol + (c.j - active_offset)*3 + d;
                        this->m_A_values[index] = this->m_dt*c.nij[d];
                        index++;

                        this->m_A_rows[index] = contacts.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 4*m_nb_gamma_min + 4*ic;
                        this->m_A_cols[index] = firstCol + (c.j - active_offset)*3 + d;
                        this->m_A_values[index] = -this->m_dt*c.nij[d];
                        index++;
                    }
                    for (std::size_t ind_row = 0; ind_row < 3; ++ind_row)
                    {
                        for (std::size_t ind_col = 0; ind_col < 3; ++ind_col)
                        {
                            this->m_A_rows[index] = contacts.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 4*ic + 1 + ind_row;
                            this->m_A_cols[index] = firstCol + (c.j - active_offset)*3 + ind_col;
                            this->m_A_values[index] = this->m_dt*m_mu*c.nij[ind_row]*c.nij[ind_col];
                            if(ind_row == ind_col)
                            {
                                this->m_A_values[index] -= this->m_dt*m_mu;
                            }
                            index++;
                        }

                        for (std::size_t ind_col = 0; ind_col < 3; ++ind_col)
                        {
                            this->m_A_rows[index] = contacts.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 4*m_nb_gamma_min + 4*ic + 1 + ind_row;
                            this->m_A_cols[index] = firstCol + (c.j - active_offset)*3 + ind_col;
                            this->m_A_values[index] = this->m_dt*m_mu*c.nij[ind_row]*c.nij[ind_col];
                            if(ind_row == ind_col)
                            {
                                this->m_A_values[index] -= this->m_dt*m_mu;
                            }
                            index++;
                        }
                    }
                }

                auto ri_cross = cross_product<dim>(c.pi - particles.pos()(c.i));
                auto rj_cross = cross_product<dim>(c.pj - particles.pos()(c.j));
                auto Ri = rotation_matrix<3>(particles.q()(c.i));
                auto Rj = rotation_matrix<3>(particles.q()(c.j));

                if (c.i >= active_offset)
                {
                    std::size_t ind_part = c.i - active_offset;
                    auto dot = xt::eval(xt::linalg::dot(ri_cross, Ri));
                    for (std::size_t ip = 0; ip < 3; ++ip)
                    {
                        this->m_A_rows[index] = contacts.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 4*ic;
                        this->m_A_cols[index] = firstCol + 3*this->m_nparticles + 3*ind_part + ip;
                        this->m_A_values[index] = this->m_dt*(c.nij[0]*dot(0, ip)+c.nij[1]*dot(1, ip)+c.nij[2]*dot(2, ip));
                        index++;

                        this->m_A_rows[index] = contacts.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 4*m_nb_gamma_min + 4*ic;
                        this->m_A_cols[index] = firstCol + 3*this->m_nparticles + 3*ind_part + ip;
                        this->m_A_values[index] = -this->m_dt*(c.nij[0]*dot(0, ip)+c.nij[1]*dot(1, ip)+c.nij[2]*dot(2, ip));
                        index++;
                    }
                    for (std::size_t ind_row = 0; ind_row < 3; ++ind_row)
                    {
                        for (std::size_t ind_col = 0; ind_col < 3; ++ind_col)
                        {
                            this->m_A_rows[index] = contacts.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 4*ic + 1 + ind_row;
                            this->m_A_cols[index] = firstCol + 3*this->m_nparticles + 3*ind_part + ind_col;
                            this->m_A_values[index] = -m_mu*this->m_dt*dot(ind_row, ind_col) + m_mu*this->m_dt*(c.nij[0]*dot(0, ind_col)+c.nij[1]*dot(1, ind_col)+c.nij[2]*dot(2, ind_col));
                            index++;
                        }

                        for (std::size_t ind_col = 0; ind_col < 3; ++ind_col)
                        {
                            this->m_A_rows[index] = contacts.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 4*m_nb_gamma_min + 4*ic + 1 + ind_row;
                            this->m_A_cols[index] = firstCol + 3*this->m_nparticles + 3*ind_part + ind_col;
                            this->m_A_values[index] = -m_mu*this->m_dt*dot(ind_row, ind_col) + m_mu*this->m_dt*(c.nij[0]*dot(0, ind_col)+c.nij[1]*dot(1, ind_col)+c.nij[2]*dot(2, ind_col));
                            index++;
                        }
                    }
                }

                if (c.j >= active_offset)
                {
                    std::size_t ind_part = c.j - active_offset;
                    auto dot = xt::eval(xt::linalg::dot(rj_cross, Rj));
                    for (std::size_t ip = 0; ip < 3; ++ip)
                    {
                        this->m_A_rows[index] = contacts.size() - m_nb_gamma_min +this-> m_nb_gamma_neg + 4*ic;
                        this->m_A_cols[index] = firstCol + 3*this->m_nparticles + 3*ind_part + ip;
                        this->m_A_values[index] = -this->m_dt*(c.nij[0]*dot(0, ip)+c.nij[1]*dot(1, ip)+c.nij[2]*dot(2, ip));
                        index++;

                        this->m_A_rows[index] = contacts.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 4*m_nb_gamma_min + 4*ic;
                        this->m_A_cols[index] = firstCol + 3*this->m_nparticles + 3*ind_part + ip;
                        this->m_A_values[index] = this->m_dt*(c.nij[0]*dot(0, ip)+c.nij[1]*dot(1, ip)+c.nij[2]*dot(2, ip));
                        index++;
                    }
                    for (std::size_t ind_row = 0; ind_row < 3; ++ind_row)
                    {
                        for (std::size_t ind_col = 0; ind_col < 3; ++ind_col)
                        {
                            this->m_A_rows[index] = contacts.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 4*ic + 1 + ind_row;
                            this->m_A_cols[index] = firstCol + 3*this->m_nparticles + 3*ind_part + ind_col;
                            this->m_A_values[index] = m_mu*this->m_dt*dot(ind_row, ind_col) - m_mu*this->m_dt*(c.nij[0]*dot(0, ind_col)+c.nij[1]*dot(1, ind_col)+c.nij[2]*dot(2, ind_col));
                            index++;
                        }

                        for (std::size_t ind_col = 0; ind_col < 3; ++ind_col)
                        {
                            this->m_A_rows[index] = contacts.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 4*m_nb_gamma_min + 4*ic + 1 + ind_row;
                            this->m_A_cols[index] = firstCol + 3*this->m_nparticles + 3*ind_part + ind_col;
                            this->m_A_values[index] = m_mu*this->m_dt*dot(ind_row, ind_col) - m_mu*this->m_dt*(c.nij[0]*dot(0, ind_col)+c.nij[1]*dot(1, ind_col)+c.nij[2]*dot(2, ind_col));
                            index++;
                        }
                    }
                }
            }
            ++ic;
        }
        // m_A_rows.resize(index);
        // m_A_cols.resize(index);
        // m_A_values.resize(index);
    }

    template<std::size_t dim>
    ViscousWithFriction<dim>::ViscousWithFriction(std::size_t nparticles, double dt)
    : base_type(nparticles, dt)
    , m_gamma_min(-3.)
    , m_mu(0.1)
    {}

    template<std::size_t dim>
    void ViscousWithFriction<dim>::set_gamma(const std::vector<neighbor<dim>>& contacts_new)
    {
        this->set_gamma_base(contacts_new);
        this->m_nb_gamma_neg = 0;
        m_nb_gamma_min = 0;
        for (auto& g : this->m_gamma)
        {
            if (g < -this->m_tol && g > m_gamma_min)
            {
                this->m_nb_gamma_neg++;
            }
            else if (g == m_gamma_min)
            {
                m_nb_gamma_min++;
            }
        }
    }

    template<std::size_t dim>
    void ViscousWithFriction<dim>::update_gamma_impl(const std::vector<neighbor<dim>>& contacts, xt::xtensor<double, 1> lambda)
    {
        this->m_contacts_old = contacts;
        this->m_gamma_old.resize(this->m_gamma.size());
        std::size_t ind_gamma_neg = 0;
        std::size_t ind_gamma_min = 0;

        for (std::size_t i = 0; i < contacts.size(); ++i)
        {
            double f_contact;
            if (this->m_gamma[i] != m_gamma_min)
            {
                if(this->m_gamma[i] < -this->m_tol)
                {
                    f_contact = lambda(i) - lambda(this->m_gamma.size() - m_nb_gamma_min + ind_gamma_neg);
                    ind_gamma_neg++;
                }
                else
                {
                    f_contact = lambda(i);
                }
            }
            else
            {
                f_contact = lambda(this->m_gamma.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 4*ind_gamma_min)
                    -  lambda(this->m_gamma.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 4*this->m_nb_gamma_min + 4*ind_gamma_min);
                ind_gamma_min++;
            }
            this->m_gamma_old[i] = std::max(m_gamma_min, std::min(0., this->m_gamma[i] - this->m_dt * f_contact));
            // for Mosek
            if (this->m_gamma_old[i] - m_gamma_min < this->m_tol)
                this->m_gamma_old[i] = m_gamma_min;
            if (this->m_gamma_old[i] > -this->m_tol)
                this->m_gamma_old[i] = 0.;
            PLOG_WARNING << this->m_gamma[i];
        }
    }

    template<std::size_t dim>
    std::size_t ViscousWithFriction<dim>::number_row_matrix_impl(const std::vector<neighbor<dim>>& contacts)
    {
        return contacts.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 2*4*m_nb_gamma_min;
    }

    template<std::size_t dim>
    void ViscousWithFriction<dim>::create_vector_distances_impl(const std::vector<neighbor<dim>>& contacts)
    {
        this->m_distances = xt::zeros<double>({contacts.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 2*4*m_nb_gamma_min});
        std::size_t index_dry = 0;
        std::size_t index_friciton = 0;
        for (std::size_t i = 0; i < contacts.size(); ++i)
        {
            if (this->m_gamma[i] != m_gamma_min)
            {
                this->m_distances[index_dry] = contacts[i].dij;
                if(this->m_gamma[i] < -this->m_tol)
                {
                    this->m_distances[contacts.size() - m_nb_gamma_min + index_dry] = -contacts[i].dij;
                }
                index_dry++;
            }
            else
            {
                this->m_distances[contacts.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 4*index_friciton] = contacts[i].dij;
                this->m_distances[contacts.size() - m_nb_gamma_min + this->m_nb_gamma_neg + 4*m_nb_gamma_min + 4*index_friciton] = -contacts[i].dij;
                index_friciton++;
            }
        }
    }

    template<std::size_t dim>
    std::size_t ViscousWithFriction<dim>::get_nb_gamma_min_impl()
    {
        return m_nb_gamma_min;
    }

}

