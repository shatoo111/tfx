#include "effects/SeasonalEffect.h"
#include <array>
#include <numbers>

namespace te {
namespace {
constexpr float tau = 2 * std::numbers::pi_v<float>;
constexpr std::array foliage{Artwork::Maple, Artwork::Plane, Artwork::Birch, Artwork::Beech};
constexpr float maxBaseLeafCount = 55.F, maxBaseStarCount = 95.F;
constexpr float leafDensity(int intensity) { return .65F + .43F * intensity; }
constexpr float starDensity(int intensity) { return .5F + .55F * intensity; }
float wrap(float v, float extent) {
    if (v >= 0 && v < extent) return v;
    return v - extent * std::floor(v / extent);
}
float smooth(float x) {
    x = std::clamp(x, 0.F, 1.F);
    return x * x * (3 - 2 * x);
}
// Petioles originate at the tips and forks of the painted bough (110 x 260).
struct Attachment { float x, y, angle, size; };
constexpr std::array<Attachment,20> tips{{
    {32,66,-1.55F,22},{41,77,-2.7F,19},{62,47,-.35F,17},{50,62,-1.05F,14},
    {103,107,.36F,19},{91,119,1.6F,16},{105,146,2.8F,17},{24,115,-1.2F,23},
    {35,106,-.3F,19},{41,146,-2.5F,22},{52,133,-1.8F,14},{15,162,-1.15F,21},
    {27,181,-2.48F,18},{36,170,-.7F,15},{65,196,.58F,18},{72,236,2.65F,20},
    {67,218,1.45F,14},{22,219,-1.9F,21},{42,247,2.9F,15},{34,243,-2.65F,21}
}};
}
SeasonalEffect::SeasonalEffect(EffectKind kind, unsigned seed) : kind_(kind), rng_(seed) {
    switch (kind_) {
    case EffectKind::Leaves:
        leaves_.reserve(std::size_t(maxBaseLeafCount * leafDensity(2)));
        frame_.illustrations.reserve(1 + tips.size() + leaves_.capacity());
        break;
    case EffectKind::Stardust:
        frame_.ambient.reserve(std::size_t(maxBaseStarCount * starDensity(2)));
        frame_.illustrations.reserve(3 * maxFlights);
        break;
    default:
        break;
    }
    clear();
}
float SeasonalEffect::range(float lo, float hi) { return std::uniform_real_distribution<float>(lo,hi)(rng_); }
void SeasonalEffect::configure(EffectConfig config) {
    config.normalize();
    if (time_ > 0 && config.flightPause != config_.flightPause)
        wait_ *= float(config.flightPause) / config_.flightPause;
    for (int i = config.flightLimit; i < groupSize_; ++i) flights_[i] = {};
    config_ = config;
    if (!config.enabled) clear();
}
void SeasonalEffect::resize(float w, float h) {
    if (w == width_ && h == height_) return;
    // Keep positions relative to the window when crossing screens or resizing.
    if (width_ > 0 && height_ > 0 && w > 0 && h > 0) {
        for (auto& leaf : leaves_) { leaf.point.x *= w/width_; leaf.point.y *= h/height_; }
        for (auto& star : frame_.ambient) { star.center.x *= w/width_; star.center.y *= h/height_; }
    }
    width_ = w; height_ = h;
    if (kind_ == EffectKind::Stardust && groupSize_ > 0 && w > 0 && h > 0) {
        const float laneHeight = height_ / groupSize_;
        for (int slot = 0; slot < groupSize_; ++slot)
            if (flights_[slot].duration > 0) fitCometTail(flights_[slot], laneHeight);
    }
}
void SeasonalEffect::clear() {
    frame_.illustrations.clear(); frame_.ambient.clear(); leaves_.clear();
    time_ = 0; flights_ = {}; groupSize_ = 0;
    wait_ = range(2.F, 4.F);
}
void SeasonalEffect::advance(float seconds, CursorSample cursor) {
    if (!needsAnimation()) { clear(); return; }
    const float dt = std::clamp(seconds, 0.F, .1F);
    time_ += dt;
    frame_.illustrations.clear();
    switch (kind_) {
    case EffectKind::Leaves: autumn(dt, cursor); break;
    case EffectKind::Stardust: stardust(dt); break;
    default: break;
    }
}
void SeasonalEffect::autumn(float dt, CursorSample cursor) {
    const float scale = std::min({1.12F, width_ * .165F / 110, height_ * .86F / 260});
    const float x = width_ + 8*scale, y = std::min(38.F, height_*.07F);
    const float sway = .017F*std::sin(time_*.68F) + .007F*std::sin(time_*1.31F);
    const float swayCos = std::cos(sway), swaySin = std::sin(sway);
    auto& art = frame_.illustrations;
    art.push_back({Artwork::Bough,x,y,110*scale,260*scale,1,0,sway});
    for (size_t i=0; i<tips.size(); ++i) {
        const auto tip = tips[i];
        const float dx = (tip.x-110)*scale, dy = tip.y*scale;
        const float size = scale*tip.size;
        art.push_back({foliage[i%4], x+dx*swayCos-dy*swaySin,
            y+dx*swaySin+dy*swayCos, size*.8F,size, .46F,.955F,
            tip.angle+sway+.072F*std::sin(time_*1.17F+float(i)*1.46F)});
    }
    const size_t count = size_t(std::clamp(width_*height_/32000.F, 5.F, maxBaseLeafCount)
                               * leafDensity(config_.ambientIntensity) * load_);
    while (leaves_.size()<count) {
        leaves_.push_back({{range(-12,width_+12),range(-14,height_+14)},range(12,24),range(0,tau),
            range(.7F,1.8F),range(19,45),range(-.85F,.85F),range(0,tau),
            foliage[std::uniform_int_distribution<int>(0,3)(rng_)]});
    }
    leaves_.resize(count);
    constexpr float cursorRadius = 60.F;
    const bool repelLeaves = cursor.inside && config_.interaction
        && std::isfinite(cursor.position.x) && std::isfinite(cursor.position.y);
    for (auto& leaf : leaves_) {
        leaf.phase += dt*leaf.frequency;
        const float flutter = std::sin(leaf.phase);
        leaf.angle += dt*(leaf.spin + .65F*flutter);
        Vec2 velocity{45*config_.wind + 19*flutter, leaf.speed + 4*std::cos(leaf.phase*.73F)};
        if (repelLeaves) {
            const float dx = leaf.point.x - cursor.position.x, dy = leaf.point.y - cursor.position.y;
            const float distanceSquared = dx*dx + dy*dy;
            if (distanceSquared < cursorRadius*cursorRadius) {
                const float distance = std::sqrt(distanceSquared);
                const float repel = 90.F * (1 - distance/cursorRadius) / std::max(1.F, distance);
                velocity.x += dx*repel;
                velocity.y += dy*repel;
            }
        }
        leaf.point.x += dt*velocity.x;
        leaf.point.y += dt*velocity.y;
        leaf.point.x = wrap(leaf.point.x+20,width_+40)-20;
        leaf.point.y = wrap(leaf.point.y+20,height_+40)-20;
        art.push_back({leaf.art,leaf.point.x,leaf.point.y,leaf.size*(.54F+.23F*std::cos(leaf.phase)),
                       leaf.size,.5F,.48F,leaf.angle});
    }
}
void SeasonalEffect::startFlight(Flight& f) {
    f.age = 0;
    f.duration = range(1.15F,1.9F);
    f.phase = range(0,tau);
    f.length = range(65,112);
    f.strength = range(.68F,.94F);
    const bool right = range(0,1) < .5F + .22F*config_.wind;
    const float end = range(.27F,.73F);
    f.from = {end+(right ? -1:1)*range(.20F,.34F),range(.30F,.43F)};
    f.to = {end, range(.60F,.73F)};
}
void SeasonalEffect::drawFlights(float dt) {
    if (groupSize_ == 0) {
        wait_ -= dt;
        if (wait_ > 0) return;
        groupSize_ = std::uniform_int_distribution<int>(1, config_.flightLimit)(rng_);
        const float laneHeight = height_ / groupSize_;
        for (int slot = 0; slot < groupSize_; ++slot) {
            auto& flight = flights_[slot];
            startFlight(flight);
            fitCometTail(flight, laneHeight);
        }
    }
    // Each flight owns its vertical band until the whole group has landed.
    // Lanes and tail bounds remain valid after window resizes.
    const float laneHeight = height_ / groupSize_;
    bool airborne = false;
    for (int slot = 0; slot < groupSize_; ++slot) {
        auto& f = flights_[slot];
        if (f.duration == 0) continue;
        f.age += dt;
        if (f.age >= f.duration + 1.18F) {
            f = {};
            continue;
        }
        airborne = true;
        drawComet(f, slot, laneHeight);
    }
    if (!airborne) {
        groupSize_ = 0;
        wait_ = float(config_.flightPause);
    }
}
void SeasonalEffect::stardust(float dt) {
    auto& stars = frame_.ambient;
    const auto count = size_t(std::clamp(width_*height_/20000.F, 8.F, maxBaseStarCount)
                              * starDensity(config_.ambientIntensity) * load_);
    while (stars.size()<count)
        stars.push_back({{range(0,width_),range(0,height_)},range(1.96F,4.76F),range(0,tau),1});
    stars.resize(count);
    for (auto& star : stars) {
        star.phase += dt*(.5F+.1F*star.radius);
        star.center.x = wrap(star.center.x+dt*(6*std::cos(star.phase)+45*config_.wind),width_);
        star.center.y = wrap(star.center.y-7*dt,height_);
        star.alpha = .55F+.39F*std::sin(star.phase);
    }
    drawFlights(dt);
}
void SeasonalEffect::fitCometTail(Flight& f, float laneHeight) const {
    const float dx = (f.to.x - f.from.x) * width_, dy = (f.to.y - f.from.y) * laneHeight;
    f.tailAngle = std::atan2(dy, dx);
    // Include the rotated tail rectangle, not just the path of its head.
    const float extentY = .93F * std::abs(std::sin(f.tailAngle)) + .095F * std::abs(std::cos(f.tailAngle));
    f.tailLength = std::min({f.length, width_ * .25F, laneHeight * .25F / extentY});
}
void SeasonalEffect::drawComet(const Flight& f, int slot, float laneHeight) {
    const float u = std::min(1.F,f.age/f.duration), after = std::max(0.F,f.age-f.duration);
    const float dx = (f.to.x-f.from.x)*width_, dy = (f.to.y-f.from.y)*laneHeight;
    const float x = f.from.x*width_ + dx*u, y = (slot + f.from.y)*laneHeight + dy*u;
    const float arrival = smooth((u-.86F)/.14F), fade = 1-smooth(after/1.18F);
    const float shine = smooth(f.age/.22F)*fade*f.strength;
    const float length = f.tailLength * smooth(f.age/.35F);
    const float core = std::min(13+7*arrival, laneHeight*.45F)*(.3F+.7F*fade);
    auto& art = frame_.illustrations;
    art.push_back({Artwork::Tail,x,y,length,length*.19F,.93F,.5F,f.tailAngle,shine*(1-smooth(after/.74F)),slot});
    art.push_back({Artwork::Halo,x,y,core,core,.5F,.5F,0,shine*(.72F+.28F*arrival),slot});
    const float rays = std::min(33.F, laneHeight*.45F)*smooth(after/.21F)*fade;
    if (rays > .05F) art.push_back({Artwork::Rays,x,y,rays*.91F,rays,.5F,.5F,-.025F,shine,slot});
}
}
