// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/vespalib/testkit/test_kit.h>
#include <vespa/searchlib/features/setup.h>
#include <vespa/searchlib/fef/test/labels.h>
#include <vespa/searchlib/features/distancefeature.h>
#include <vespa/searchlib/fef/test/dummy_dependency_handler.h>
#include <vespa/searchlib/test/features/distance_closeness_fixture.h>
#include <vespa/vespalib/stllike/asciistream.h>
#include <vespa/vespalib/util/stringfmt.h>

using search::feature_t;
using namespace search::features::test;
using namespace search::features;
using namespace search::fef::test;
using namespace search::fef;

const vespalib::string labelFeatureName("distance(label,nns)");
const vespalib::string fieldFeatureName("distance(bar)");

using RankFixture = DistanceClosenessFixture;

TEST_F("require that blueprint can be created from factory", BlueprintFactoryFixture) {
    Blueprint::SP bp = f.factory.createBlueprint("distance");
    EXPECT_TRUE(bp.get() != 0);
    EXPECT_TRUE(dynamic_cast<DistanceBlueprint*>(bp.get()) != 0);
}

TEST_FFF("require that no features are dumped", DistanceBlueprint, IndexEnvironmentFixture, FeatureDumpFixture) {
    f1.visitDumpFeatures(f2.indexEnv, f3);
}

TEST_FF("require that setup can be done on random label", DistanceBlueprint, IndexEnvironmentFixture) {
    DummyDependencyHandler deps(f1);
    f1.setName(vespalib::make_string("%s(label,random_label)", f1.getBaseName().c_str()));
    EXPECT_TRUE(static_cast<Blueprint&>(f1).setup(f2.indexEnv, std::vector<vespalib::string>{"label", "random_label"}));
}

TEST_FF("require that setup with unknown field fails", DistanceBlueprint, IndexEnvironmentFixture) {
    DummyDependencyHandler deps(f1);
    f1.setName(vespalib::make_string("%s(field,random_fieldname)", f1.getBaseName().c_str()));
    EXPECT_FALSE(static_cast<Blueprint&>(f1).setup(f2.indexEnv, std::vector<vespalib::string>{"field", "random_fieldname"}));
}

TEST_FF("require that no label gives max-double distance", NoLabel(), RankFixture(2, 2, f1, labelFeatureName)) {
    EXPECT_EQUAL(std::numeric_limits<feature_t>::max(), f2.getScore(10));
}

TEST_FF("require that unrelated label gives max-double distance", SingleLabel("unrelated", 1), RankFixture(2, 2, f1, labelFeatureName)) {
    EXPECT_EQUAL(std::numeric_limits<feature_t>::max(), f2.getScore(10));
}

TEST_FF("require that labeled item raw score can be obtained", SingleLabel("nns", 1), RankFixture(2, 2, f1, labelFeatureName)) {
    f2.setFooScore(0, 10, 5.0);
    EXPECT_EQUAL(5.0, f2.getScore(10));
}

TEST_FF("require that field raw score can be obtained", NoLabel(), RankFixture(2, 2, f1, fieldFeatureName)) {
    f2.setBarScore(0, 10, 5.0);
    EXPECT_EQUAL(5.0, f2.getScore(10));
}

TEST_FF("require that other raw scores are ignored", SingleLabel("nns", 2), RankFixture(2, 2, f1, labelFeatureName)) {
    f2.setFooScore(0, 10, 1.0);
    f2.setFooScore(1, 10, 2.0);
    f2.setBarScore(0, 10, 5.0);
    f2.setBarScore(1, 10, 6.0);
    EXPECT_EQUAL(2.0, f2.getScore(10));
}

TEST_FF("require that the correct raw score is used", NoLabel(), RankFixture(2, 2, f1, fieldFeatureName)) {
    f2.setFooScore(0, 10, 3.0);
    f2.setFooScore(1, 10, 4.0);
    f2.setBarScore(0, 10, 8.0);
    f2.setBarScore(1, 10, 7.0);
    EXPECT_EQUAL(7.0, f2.getScore(10));
}

TEST_FF("require that stale data is ignored", SingleLabel("nns", 2), RankFixture(2, 2, f1, labelFeatureName)) {
    f2.setFooScore(0, 10, 1.0);
    f2.setFooScore(1, 5, 2.0);
    EXPECT_EQUAL(std::numeric_limits<feature_t>::max(), f2.getScore(10));
}

TEST_MAIN() { TEST_RUN_ALL(); }
