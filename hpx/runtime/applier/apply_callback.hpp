//  Copyright (c) 2007-2015 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_APPLIER_APPLY_CALLBACK_DEC_16_2012_1228PM)
#define HPX_APPLIER_APPLY_CALLBACK_DEC_16_2012_1228PM

#include <hpx/hpx_fwd.hpp>
#include <hpx/exception.hpp>

#include <hpx/runtime/applier/apply.hpp>

#include <boost/make_shared.hpp>

namespace hpx
{
    ///////////////////////////////////////////////////////////////////////////
    namespace applier { namespace detail
    {
        template <typename Action, typename Callback, typename ...Ts>
        inline bool
        apply_r_p_cb(naming::address&& addr, naming::id_type const& id,
            threads::thread_priority priority, Callback && cb, Ts&&... vs)
        {
            // If remote, create a new parcel to be sent to the destination
            // Create a new parcel with the gid, action, and arguments
            lcos::local::detail::invoke_when_ready(
                detail::put_parcel<Action>(id, std::move(addr), priority,
                    std::unique_ptr<actions::continuation>(), std::forward<Callback>(cb)),
                std::forward<Ts>(vs)...);
            return false;     // destinations are remote
        }

        template <typename Action, typename Callback, typename ...Ts>
        inline bool
        apply_r_cb(naming::address&& addr, naming::id_type const& gid,
            Callback && cb, Ts&&... vs)
        {
            return apply_r_p_cb<Action>(std::move(addr), gid,
                actions::action_priority<Action>(), std::forward<Callback>(cb),
                std::forward<Ts>(vs)...);
        }
    }}

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename Callback, typename ...Ts>
    inline bool
    apply_p_cb(naming::id_type const& gid, threads::thread_priority priority,
        Callback && cb, Ts&&... vs)
    {
        return hpx::detail::apply_cb_impl<Action>(
            std::unique_ptr<actions::continuation>(), gid, priority,
            std::forward<Callback>(cb), std::forward<Ts>(vs)...);
    }

    template <typename Action, typename Callback, typename ...Ts>
    inline bool
    apply_cb(naming::id_type const& gid, Callback && cb, Ts&&... vs)
    {
        return apply_p_cb<Action>(gid, actions::action_priority<Action>(),
            std::forward<Callback>(cb), std::forward<Ts>(vs)...);
    }

    template <typename Component, typename Signature, typename Derived,
        typename Callback, typename ...Ts>
    inline bool
    apply_cb(
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        naming::id_type const& gid, Callback && cb, Ts&&... vs)
    {
        return apply_p_cb<Derived>(gid, actions::action_priority<Derived>(),
            std::forward<Callback>(cb), std::forward<Ts>(vs)...);
    }

    template <typename Action, typename DistPolicy, typename Callback,
        typename ...Ts>
    inline typename boost::enable_if_c<
        traits::is_distribution_policy<DistPolicy>::value, bool
    >::type
    apply_p_cb(DistPolicy const& policy, threads::thread_priority priority,
        Callback && cb, Ts&&... vs)
    {
        return policy.template apply_cb<Action>(
            std::unique_ptr<actions::continuation>(), priority,
            std::forward<Callback>(cb), std::forward<Ts>(vs)...);
    }

    template <typename Action, typename DistPolicy, typename Callback,
        typename ...Ts>
    inline typename boost::enable_if_c<
        traits::is_distribution_policy<DistPolicy>::value, bool
    >::type
    apply_cb(DistPolicy const& policy, Callback && cb, Ts&&... vs)
    {
        return apply_p_cb<Action>(policy, actions::action_priority<Action>(),
            std::forward<Callback>(cb), std::forward<Ts>(vs)...);
    }

    template <typename Component, typename Signature, typename Derived,
        typename DistPolicy, typename Callback, typename ...Ts>
    inline typename boost::enable_if_c<
        traits::is_distribution_policy<DistPolicy>::value, bool
    >::type
    apply_cb(
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        DistPolicy const& policy, Callback && cb, Ts&&... vs)
    {
        return apply_p_cb<Derived>(policy, actions::action_priority<Derived>(),
            std::forward<Callback>(cb), std::forward<Ts>(vs)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace applier { namespace detail
    {
        template <typename Action, typename Callback, typename ...Ts>
        inline bool
        apply_r_p_cb(naming::address&& addr,
            std::unique_ptr<actions::continuation> c, naming::id_type const& id,
            threads::thread_priority priority, Callback && cb, Ts&&... vs)
        {
            // If remote, create a new parcel to be sent to the destination
            // Create a new parcel with the gid, action, and arguments
            lcos::local::detail::invoke_when_ready(
                detail::put_parcel<Action>(id, std::move(addr), priority, std::move(c),
                    std::forward<Callback>(cb)),
                std::forward<Ts>(vs)...);
            return false;     // destination is remote
        }

        template <typename Action, typename Callback, typename ...Ts>
        inline bool
        apply_r_cb(naming::address&& addr, std::unique_ptr<actions::continuation> c,
            naming::id_type const& gid, Callback && cb, Ts&&... vs)
        {
            return apply_r_p_cb<Action>(std::move(addr), std::move(c), gid,
                actions::action_priority<Action>(), std::forward<Callback>(cb),
                std::forward<Ts>(vs)...);
        }
    }}

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename Callback, typename ...Ts>
    inline bool
    apply_p_cb(std::unique_ptr<actions::continuation> c, naming::address&& addr,
        naming::id_type const& gid, threads::thread_priority priority,
        Callback && cb, Ts&&... vs)
    {
        if (!traits::action_is_target_valid<Action>::call(gid)) {
            HPX_THROW_EXCEPTION(bad_parameter, "apply_p_cb",
                boost::str(boost::format(
                    "the target (destination) does not match the action type (%s)"
                ) % hpx::actions::detail::get_action_name<Action>()));
            return false;
        }

        // Determine whether the gid is local or remote
        if (addr.locality_ == hpx::get_locality()) {
            // apply locally
            bool result = applier::detail::apply_l_p<Action>(std::move(c), gid,
                std::move(addr), priority, std::forward<Ts>(vs)...);

            // invoke callback
            cb(boost::system::error_code(), parcelset::parcel());
            return result;
        }

        // apply remotely
        return applier::detail::apply_r_p_cb<Action>(std::move(addr), std::move(c), gid,
            priority, std::forward<Callback>(cb), std::forward<Ts>(vs)...);
    }

    template <typename Action, typename Callback, typename ...Ts>
    inline bool
    apply_p_cb(std::unique_ptr<actions::continuation> c, naming::id_type const& gid,
        threads::thread_priority priority, Callback && cb, Ts&&... vs)
    {
        return hpx::detail::apply_cb_impl<Action>(std::move(c), gid, priority,
            std::forward<Callback>(cb), std::forward<Ts>(vs)...);
    }

    template <typename Action, typename Callback, typename ...Ts>
    inline bool
    apply_cb(std::unique_ptr<actions::continuation> c, naming::id_type const& gid,
        Callback && cb, Ts&&... vs)
    {
        return apply_p_cb<Action>(std::move(c), gid, actions::action_priority<Action>(),
            std::forward<Callback>(cb), std::forward<Ts>(vs)...);
    }

    template <typename Component, typename Signature, typename Derived,
        typename Callback, typename ...Ts>
    inline bool
    apply_cb(std::unique_ptr<actions::continuation> c,
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        naming::id_type const& gid, Callback && cb, Ts&&... vs)
    {
        return apply_p<Derived>(std::move(c), gid, actions::action_priority<Derived>(),
            std::forward<Callback>(cb), std::forward<Ts>(vs)...);
    }

    template <typename Action, typename DistPolicy, typename Callback,
        typename ...Ts>
    inline typename boost::enable_if_c<
        traits::is_distribution_policy<DistPolicy>::value, bool
    >::type
    apply_p_cb(std::unique_ptr<actions::continuation> c, DistPolicy const& policy,
        threads::thread_priority priority, Callback && cb, Ts&&... vs)
    {
        return policy.template apply_cb<Action>(
            std::move(c), priority, std::forward<Callback>(cb), std::forward<Ts>(vs)...);
    }

    template <typename Action, typename DistPolicy, typename Callback,
        typename ...Ts>
    inline typename boost::enable_if_c<
        traits::is_distribution_policy<DistPolicy>::value, bool
    >::type
    apply_cb(std::unique_ptr<actions::continuation> c, DistPolicy const& policy,
        Callback && cb, Ts&&... vs)
    {
        return apply_p_cb<Action>(std::move(c), policy, actions::action_priority<Action>(),
            std::forward<Callback>(cb), std::forward<Ts>(vs)...);
    }

    template <typename Component, typename Signature, typename Derived,
        typename DistPolicy, typename Callback, typename ...Ts>
    inline typename boost::enable_if_c<
        traits::is_distribution_policy<DistPolicy>::value, bool
    >::type
    apply_cb(std::unique_ptr<actions::continuation> c,
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        DistPolicy const& policy, Callback && cb, Ts&&... vs)
    {
        return apply_p<Derived>(std::move(c), policy, actions::action_priority<Derived>(),
            std::forward<Callback>(cb), std::forward<Ts>(vs)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace applier { namespace detail
    {
        template <typename Action, typename Callback, typename ...Ts>
        inline bool
        apply_c_p_cb(naming::address&& addr, naming::id_type const& contgid,
            naming::id_type const& gid, threads::thread_priority priority,
            Callback && cb, Ts&&... vs)
        {
            typedef
                typename hpx::actions::extract_action<Action>::result_type
                result_type;

            std::unique_ptr<actions::typed_continuation<result_type> > cont
                = new actions::typed_continuation<result_type>(contgid);
            return apply_r_p_cb<Action>(std::move(addr),
                std::move(cont),
                gid, priority, std::forward<Callback>(cb),
                std::forward<Ts>(vs)...);
        }

        template <typename Action, typename Callback, typename ...Ts>
        inline bool
        apply_c_cb(naming::address&& addr, naming::id_type const& contgid,
            naming::id_type const& gid, Callback && cb, Ts&&... vs)
        {
            typedef
                typename hpx::actions::extract_action<Action>::result_type
                result_type;

            std::unique_ptr<actions::continuation> cont(new actions::typed_continuation<result_type>(contgid));
            return apply_r_p_cb<Action>(std::move(addr),
                std::move(cont),
                gid, actions::action_priority<Action>(),
                std::forward<Callback>(cb),
                std::forward<Ts>(vs)...);
        }
    }}

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename Callback, typename ...Ts>
    inline bool
    apply_c_p_cb(naming::id_type const& contgid, naming::id_type const& gid,
        threads::thread_priority priority, Callback && cb, Ts&&... vs)
    {
        typedef
            typename hpx::actions::extract_action<Action>::result_type
            result_type;

        std::unique_ptr<actions::continuation> cont(new actions::typed_continuation<result_type>(contgid));
        return apply_p_cb<Action>(
            std::move(cont),
            gid, priority, std::forward<Callback>(cb),
            std::forward<Ts>(vs)...);
    }

    template <typename Action, typename Callback, typename ...Ts>
    inline bool
    apply_c_cb(naming::id_type const& contgid, naming::id_type const& gid,
        Callback && cb, Ts&&... vs)
    {
        typedef
            typename hpx::actions::extract_action<Action>::result_type
            result_type;

        std::unique_ptr<actions::continuation> cont(new actions::typed_continuation<result_type>(contgid));
        return apply_p_cb<Action>(
            std::move(cont),
            gid, actions::action_priority<Action>(),
            std::forward<Callback>(cb), std::forward<Ts>(vs)...);
    }

    template <typename Action, typename Callback, typename ...Ts>
    inline bool
    apply_c_p_cb(naming::id_type const& contgid, naming::address&& addr,
        naming::id_type const& gid, threads::thread_priority priority,
        Callback && cb, Ts&&... vs)
    {
        typedef
            typename hpx::actions::extract_action<Action>::result_type
            result_type;

        std::unique_ptr<actions::continuation> cont(new actions::typed_continuation<result_type>(contgid));
        return apply_p_cb<Action>(
            std::move(cont),
            std::move(addr), gid, priority, std::forward<Callback>(cb),
            std::forward<Ts>(vs)...);
    }

    template <typename Action, typename Callback, typename ...Ts>
    inline bool
    apply_c_cb(naming::id_type const& contgid, naming::address&& addr,
        naming::id_type const& gid, Callback && cb, Ts&&... vs)
    {
        typedef
            typename hpx::actions::extract_action<Action>::result_type
            result_type;

        std::unique_ptr<actions::continuation> cont(new actions::typed_continuation<result_type>(contgid));
        return apply_p_cb<Action>(
            std::move(cont),
            std::move(addr), gid, actions::action_priority<Action>(),
            std::forward<Callback>(cb), std::forward<Ts>(vs)...);
    }
}

#endif
