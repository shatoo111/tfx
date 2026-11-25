#include "effects/SakuraEffect.h"
#include "effects/EffectFactory.h"
#include <cstdlib>
#include <iostream>
#include <limits>

void check(bool ok, const char* message) {
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}
void step(te::IEffect& effect, int frames) { for (int i=0;i<frames;++i) effect.advance(1.F/60.F,{}); }
int main() {
    te::EffectConfig config; config.kind=te::EffectKind::Sakura; config.sakuraAngle=0;
    auto factory=te::createEffect(config.kind); factory->configure(config); factory->resize(800,600);
    const auto isBlossom = [](const te::Illustration& part) {
        return part.art == te::Artwork::CherryBloom || part.art == te::Artwork::CherryRose;
    };
    for (const int side : {0, 1, 2}) {
        auto branchConfig = config;
        branchConfig.branchSide = side;
        branchConfig.branchOpacity = 0;
        factory->configure(branchConfig);
        bool left = false, right = false;
        for (const auto& part : factory->frame().illustrations) {
            check(part.opacity >= te::EffectConfig::minBranchOpacity, "Branch opacity must respect the visibility floor.");
            left |= part.art == te::Artwork::SakuraLeft;
            right |= part.art == te::Artwork::SakuraRight;
        }
        check(left == (side != 1) && right == (side != 0), "Branches must respect the side setting.");
    }
    for (const auto viewport : {te::Vec2{400, 300}, te::Vec2{1200, 600}, te::Vec2{640, 174}}) {
        config.branchWidth = 160;
        factory->configure(config);
        factory->resize(viewport.x, viewport.y);
        const te::Illustration* left = nullptr;
        const te::Illustration* right = nullptr;
        for (const auto& part : factory->frame().illustrations) {
            if (part.art == te::Artwork::SakuraLeft) left = &part;
            if (part.art == te::Artwork::SakuraRight) right = &part;
            if (isBlossom(part)) {
                check(part.y < viewport.y / 3.F, "Flowers must stay in the upper corners.");
                check(part.x < viewport.x / 4.F || part.x > viewport.x * .75F, "Flowers must leave the centre clear.");
            }
        }
        check(left && right && left->x == 0 && right->x == viewport.x && left->y == 0 && right->y == 0,
              "Both ink branches must attach to their own top corners.");
        check(left->width > -right->width && left->height > right->height, "The left branch must be larger.");
        check(left->width <= viewport.x / 4.F + .01F && left->height <= viewport.y / 3.F + .01F,
              "The branch must fit its corner at maximum size.");
        check(std::abs(left->width / left->height - 240.F / 170.F) < .001F, "Resizing must retain the artwork proportions.");
        factory->clear();
        step(*factory, 1);
        bool sawEmission = false;
        for (int i = 0; i < 120; ++i) {
            const auto previousCount = factory->frame().petals.size();
            step(*factory, 1);
            if (factory->frame().petals.size() <= previousCount) continue;
            const auto position = factory->frame().petals.back().position;
            check(position.y >= -10.F && position.y < -8.F && position.x >= -1.F && position.x <= viewport.x + 1.F,
                  "New petals must enter from above the viewport while branches are visible.");
            sawEmission = true;
            break;
        }
        check(sawEmission, "Petals must continue to spawn after resizing.");
    }
    config.branchWidth = 78;
    te::SakuraEffect immediate(8);
    config.sakuraBranches=false; immediate.configure(config); immediate.resize(900,600);
    immediate.advance(1.F/60.F,{});
    check(!immediate.frame().petals.empty(),"First frame must show petals even with branches off.");
    config.sakuraBranches=true;
    std::size_t previous=0;
    for (int intensity=0;intensity<3;++intensity) {
        te::SakuraEffect effect(7); config.sakuraIntensity=intensity;
        effect.configure(config); effect.resize(900,600); step(effect,240);
        check(effect.frame().petals.size()>previous,"Density presets must increase petal count.");
        previous=effect.frame().petals.size();
        check(effect.frame().snow.empty() && effect.frame().rain.empty(),"Sakura must not leak other effects.");
        config.sakuraBranches=false; effect.configure(config);
        check(effect.frame().illustrations.empty() && !effect.frame().petals.empty(),"Branch switch must preserve falling petals.");
        config.sakuraBranches=true; config.enabled=false; effect.configure(config);
        check(!effect.needsAnimation() && effect.frame().petals.empty() && effect.frame().illustrations.empty(),"Off must stop and hide Sakura.");
        config.enabled=true;
    }
    te::SakuraEffect natural(12), touched(12), ignored(12);
    for (auto* e:{&natural,&touched,&ignored}) { e->configure(config); e->resize(900,600); step(*e,120); }
    config.interaction=false; ignored.configure(config); config.interaction=true;
    for (int i=0;i<60;++i) {
        const auto p=natural.frame().petals.front().position;
        te::CursorSample cursor{{p.x-12,p.y},true};
        natural.advance(1.F/60.F,{}); touched.advance(1.F/60.F,cursor); ignored.advance(1.F/60.F,cursor);
    }
    check(touched.frame().petals.front().position.x>natural.frame().petals.front().position.x+8,"Cursor must repel petals.");
    check(std::abs(ignored.frame().petals.front().position.x-natural.frame().petals.front().position.x)<0.001F,"Interaction off must ignore cursor.");
    te::SakuraEffect left(5),right(5);
    for (auto* e:{&left,&right}) { e->configure(config); e->resize(900,600); step(*e,120); }
    config.sakuraAngle=-60; left.configure(config); config.sakuraAngle=60; right.configure(config);
    step(left,60);step(right,60);
    check(right.frame().petals.front().position.x>left.frame().petals.front().position.x+60,"Angle must control horizontal flight.");
    for (float angle : {-60.F, 60.F}) {
        te::SakuraEffect fromTop(7);
        config.sakuraAngle = angle;
        fromTop.configure(config);
        fromTop.resize(400, 600);
        step(fromTop, 30);
        check(!fromTop.frame().petals.empty(), "Petals must start spawning immediately.");
        const auto initialCount = fromTop.frame().petals.size();
        check(std::any_of(fromTop.frame().petals.begin(), fromTop.frame().petals.end(),
                          [](const auto& petal) { return petal.position.y > 300; }),
              "First frames must distribute petals across the window rather than spawning a wave.");
        step(fromTop, 1200);
        check(fromTop.frame().petals.size() >= initialCount / 3,
              "Density must persist after the original petals have fallen.");
    }
    te::SakuraEffect fast(32), slow(32);
    for (auto* e:{&fast,&slow}) { e->configure(config); e->resize(900,600); }
    for(int i=0;i<240;++i) fast.advance(1.F/60.F,{});
    for(int i=0;i<120;++i) slow.advance(1.F/30.F,{});
    check(fast.frame().petals.size()==slow.frame().petals.size(),"30/60 fps density must match.");
    check(std::abs(fast.frame().petals.front().position.x-slow.frame().petals.front().position.x)<0.1F,"30/60 fps trajectories must match.");
    fast.setLoadScale(0.3F); step(fast,36000);
    check(fast.frame().petals.size()<=54,"Adaptive load must bound long-run particle count.");
    fast.advance(std::numeric_limits<float>::quiet_NaN(),{});
    for(const auto& p:fast.frame().petals) check(std::isfinite(p.position.x)&&std::isfinite(p.position.y),"Invalid dt must not poison geometry.");
    fast.clear(); check(fast.frame().petals.empty(),"Clear removes airborne petals.");
    fast.resize(0,0); check(!fast.needsAnimation()&&fast.frame().illustrations.empty(),"Empty windows stop.");
    config.sakuraAngle=std::numeric_limits<float>::infinity();config.normalize();
    check(config.sakuraAngle==15,"Invalid angles normalize safely.");
    std::cout << "Sakura simulation checks passed\n";
}
