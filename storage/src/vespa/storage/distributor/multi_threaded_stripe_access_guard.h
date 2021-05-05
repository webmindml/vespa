// Copyright Verizon Media. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "stripe_access_guard.h"

namespace storage::distributor {

class MultiThreadedStripeAccessor;
class DistributorStripePool;
class DistributorStripe;

/**
 * StripeAccessGuard implementation which provides exclusive access to a set of stripes
 * by ensuring that all stripe threads are safely parked upon guard construction. This
 * means that as long as a guard exists, access to stripes is guaranteed to not cause
 * data races.
 *
 * Threads are automatically un-parked upon guard destruction.
 *
 * At most one guard instance may exist at any given time.
 */
class MultiThreadedStripeAccessGuard : public StripeAccessGuard {
    MultiThreadedStripeAccessor& _accessor;
    DistributorStripePool&       _stripe_pool;
public:
    MultiThreadedStripeAccessGuard(MultiThreadedStripeAccessor& accessor,
                                   DistributorStripePool& stripe_pool);
    ~MultiThreadedStripeAccessGuard() override;

    void flush_and_close() override;

    void update_total_distributor_config(std::shared_ptr<const DistributorConfiguration> config) override;

    void update_distribution_config(const BucketSpaceDistributionConfigs& new_configs) override;
    void set_pending_cluster_state_bundle(const lib::ClusterStateBundle& pending_state) override;
    void clear_pending_cluster_state_bundle() override;
    void enable_cluster_state_bundle(const lib::ClusterStateBundle& new_state) override;
    void notify_distribution_change_enabled() override;

    PotentialDataLossReport remove_superfluous_buckets(document::BucketSpace bucket_space,
                                                       const lib::ClusterState& new_state,
                                                       bool is_distribution_change) override;
    void merge_entries_into_db(document::BucketSpace bucket_space,
                               api::Timestamp gathered_at_timestamp,
                               const lib::Distribution& distribution,
                               const lib::ClusterState& new_state,
                               const char* storage_up_states,
                               const std::unordered_set<uint16_t>& outdated_nodes,
                               const std::vector<dbtransition::Entry>& entries) override;

    void update_read_snapshot_before_db_pruning() override;
    void update_read_snapshot_after_db_pruning(const lib::ClusterStateBundle& new_state) override;
    void update_read_snapshot_after_activation(const lib::ClusterStateBundle& activated_state) override;
    void clear_read_only_bucket_repo_databases() override;
private:
    // TODO STRIPE remove once multi threaded stripe support is implemented
    DistributorStripe& first_stripe() noexcept;
};

/**
 * Impl of StripeAccessor which creates MultiThreadedStripeAccessGuards that cover all threads
 * in the provided stripe pool.
 */
class MultiThreadedStripeAccessor : public StripeAccessor {
    DistributorStripePool& _stripe_pool;
    bool                   _guard_held;

    friend class MultiThreadedStripeAccessGuard;
public:
    explicit MultiThreadedStripeAccessor(DistributorStripePool& stripe_pool)
        : _stripe_pool(stripe_pool),
          _guard_held(false)
    {}
    ~MultiThreadedStripeAccessor() override = default;

    std::unique_ptr<StripeAccessGuard> rendezvous_and_hold_all() override;
private:
    void mark_guard_released();
};

}
