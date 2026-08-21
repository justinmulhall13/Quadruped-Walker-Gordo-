#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Preferences.h>
#include <Adafruit_PWMServoDriver.h>

const char* AP_SSID = "QuadrupedTuner";
const char* AP_PASS = "12345678";

WebServer server(80);
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40);

float angles[8] = {};

static int angleToUs(float deg) {
  return constrain(1500 + (int)(deg * 7.407f), 500, 2500);
}

static void setServo(int ch, float deg) {
  angles[ch] = constrain(deg, -135.0f, 135.0f);
  pca.writeMicroseconds(ch, angleToUs(angles[ch]));
}

static void setPose(const float a[8]) {
  for (int i = 0; i < 8; i++) setServo(i, a[i]);
}

// ---------- channel mapping ----------
// Verified on hardware 2026-07-29
#define FL_KNEE  0
#define FL_HIP   1
#define FR_KNEE  2
#define FR_HIP   3
#define BR_KNEE  4
#define BR_HIP   5
#define BL_KNEE  6
#define BL_HIP   7

const float POSE_STAND[8] = {-103, 33, 77, -43, 101, 63, -78, -39};

// Legs splayed out flat on the floor — the base pose for the worm.
const float POSE_FLAT[8]  = {-21, 84, -5, -99, 26, 114, -6, -84};

// ---------- gait model ----------
// Crawl gait: exactly one leg is airborne at a time; the three planted legs
// sweep their hips backward together. The stance group moving in lockstep is
// what drives the body forward.
//
// Steering is differential stride: the left pair and right pair get opposite
// stride offsets, so the robot turns. Hips only yaw fore/aft, so there is no
// lateral strafe on this hardware — "left/right" is rotation.
//
// Signs measured on hardware 2026-07-29: kneeSign lifts the foot, hipSign
// swings the leg forward. side is +1 for the left pair, -1 for the right pair.
#define LEG_FL 0
#define LEG_FR 1
#define LEG_BR 2
#define LEG_BL 3

struct LegConfig {
  int   kneeCh, hipCh;
  float kneeSign, hipSign;
  float side;
  float phaseOffset;   // where this leg's swing falls in the cycle, 0..1
  float kneeLift;
  float frontSign;     // +1 front pair, -1 back pair: which way the foot's
                       // reach has to change as the body drives past it
};

LegConfig legs[4];

// Tuned on hardware 2026-08-19. Stride 55 and body drop 5 came out of live
// tuning; stride is now split by axis, see below.
//
// Turning and walking are not equally kind to this leg design. A hip is a yaw
// joint, so a planted foot can only trace an arc around that hip. When the
// robot spins, that arc IS the motion the foot needs — every foot runs
// tangentially around the body center, so nothing fights and the push is clean.
// Walking straight asks each planted foot to trace a straight line instead, and
// three stance feet on three incompatible arcs scrub sideways until the
// least-grippy one skids. The scrub goes as (1 - cos(stride/2)), so it grows
// with roughly the square of stride — halving the drive stride cuts it ~4x.
//
// So: keep the full 55 for turning where it costs nothing, use a shorter stride
// for driving, and run a faster cycle so top speed stays about the same.
// 32/1100ms traded away too much speed on the straights, so give the stride
// back most of the way and pay for it with a faster cycle: 40 per 950ms sweeps
// ~42 deg/s vs the original 55 per 1600ms at ~34 deg/s, and still scrubs ~25%
// less per step than stride 55 did.
const float STRIDE_TURN = 55;

// Drive stride, cut hard from 40 on 2026-08-21 and now runtime-adjustable.
// A 40 deg hip sweep asks the body to travel further in one step than the knee
// can pay for in reach (see REACH_RATIO) - the surplus comes out as skid. A
// short stride that actually lands beats a long one that scrubs half of itself
// away.
const float STRIDE_FWD  = 20;
float strideFwdCmd      = STRIDE_FWD;

// Straight-line stance correction, in knee degrees per hip degree.
//
// A hip is a yaw joint, so hip motion alone drags a planted foot around an arc
// centred on that hip. For the body to translate instead, each planted foot has
// to travel a straight line in body frame, and that needs a RADIAL component
// too - the knee is exactly the joint that changes a foot's reach. With the
// legs splayed at ~45 deg the radial and tangential halves of that line are
// equal, so the knee has to sweep about (hip radius / shin length) times as far
// as the hip does. Front feet pull in as the body drives past them, back feet
// reach out - hence LegConfig::frontSign.
//
// 1.2 is the geometric estimate for this build; tune it live and watch the
// feet. 0 reproduces the old arc-only stance.
const float REACH_RATIO = 1.2f;
float reachRatioCmd     = REACH_RATIO;

const float BODY_DROP   = 5;

// The back pair stands more splayed than the front pair, so their feet start
// the push too far out from the body. Pull them in a touch. kneeOff is
// positive-outward, so "more vertical" SUBTRACTS. The stance sweep itself is
// untouched - this only shifts where that sweep starts and ends.
const float BACK_KNEE_TILT = 5;
// Slowed from 950ms on 2026-08-21. A taller lift needs more milliseconds of
// swing to happen in: at 950/0.22 the whole swing was 209ms, and a DS3218
// simply lags behind a command that big in that time, so the commanded lift
// never made it to the foot.
const int   CYCLE_MS    = 1150;

// Legs are 0.25 apart in phase, so duty MUST stay under 0.25 — at 0.28 two legs
// were airborne at once for ~48ms every step, dropping the body onto two feet
// right at the plant. 0.22 leaves a short all-feet-down window between swings.
const float SWING_DUTY = 0.22f;

// Foot lift in degrees of knee. 22 still scuffed; 32 is the new starting point.
// Adjustable live from the web UI and saved to flash, because how much lift is
// enough depends on the floor and is only judged by eye.
const float KNEE_LIFT = 32;
float kneeLiftCmd     = KNEE_LIFT;

// Hold the hip still for the first slice of stance and press the knee down past
// the stance height, so the foot is loaded and gripping before it starts to
// push instead of pushing the instant it touches down. Note the sign: kneeOff
// is positive-up, so a press has to SUBTRACT. It used to add, which lifted the
// newly planted foot ~4 deg right as it was meant to be taking weight.
const float STANCE_SETTLE = 0.10f;
const float FOOT_PRESS    = 4;

const int UPDATE_MS      = 20;
const int DRIVE_TIMEOUT  = 700;   // stop if the browser stops sending input

enum Mode { MODE_IDLE, MODE_DRIVE, MODE_EMOTE };
Mode mode = MODE_IDLE;

float driveY = 0;      // +forward / -backward
float driveX = 0;      // +right turn / -left turn

// Straight-line trim. The four legs never push exactly equally — different
// friction, slop and load per corner — so the yaw impulses don't cancel over a
// cycle and the robot arcs instead of tracking straight. This is a constant
// differential-stride bias that cancels it: positive corrects a leftward drift.
// Scaled by signed drive, so reversing flips the correction with the error.
// Saved to flash, so it survives power cycles.
float driveTrim = 0;
Preferences prefs;
float gaitPhase  = 0;  // 0..1 through the gait cycle

// Backward is the forward gait mirrored front-to-back: every leg takes over
// its fore-aft partner.s slot, so the touchdown order reverses from
// FR,BL,FL,BR to BR,FL,BL,FR. The hip sweep and the reach correction already
// invert on their own (both scale with a now-negative strideFwdPart) - the
// sequence is the one piece that does not, and running a forward crawl order
// backwards lifts the wrong leg for the support triangle, which is why
// reverse went nowhere. Indices are FL,FR,BR,BL, so this maps FL<->BL, FR<->BR.
const int PHASE_MIRROR[4] = { LEG_BL, LEG_BR, LEG_FR, LEG_FL };
bool gaitRev = false;   // committed only at a cycle boundary, see loop()
bool  stopping   = false;
unsigned long lastDriveMs = 0;
unsigned long lastUpdate  = 0;

int pendingEmote = 0;  // 1 = wave, 2 = worm

static float fracf(float x) { return x - floorf(x); }

void poseStand(float out[8]) {
  for (int i = 0; i < 4; i++) {
    LegConfig &L = legs[i];
    out[L.kneeCh] = POSE_STAND[L.kneeCh] + L.kneeSign * BODY_DROP;
    out[L.hipCh]  = POSE_STAND[L.hipCh];
  }
}

// Foot height through the swing, as a fraction of kneeLift. A plain sin(PI*w)
// is only clear of the floor near midswing: the toe is still down as the hip
// starts forward, and back down before the hip has finished — both ends scuff,
// which is what reads as shuffling. Snap up, hold clear across the middle of
// the step, then ease back down.
static float liftProfile(float w) {
  const float UP = 0.30f, DOWN = 0.30f;
  if (w < UP)          return 0.5f * (1.0f - cosf(PI * w / UP));
  if (w < 1.0f - DOWN) return 1.0f;
  float d = (w - (1.0f - DOWN)) / DOWN;
  return 0.5f * (1.0f + cosf(PI * d));
}

void computeDrivePose(float p, float fwd, float turn, float out[8]) {
  float turnEff = turn + driveTrim * fwd;   // hold a straight line

  for (int i = 0; i < 4; i++) {
    LegConfig &L = legs[i];
    // Keep the two halves of the command apart: only the translation half needs
    // the radial correction below. Spinning wants the plain arc, because when
    // the robot turns in place the arc IS the path the foot should take.
    float strideFwdPart  = strideFwdCmd * fwd;
    float strideTurnPart = STRIDE_TURN * turnEff * L.side;
    float stride = constrain(strideFwdPart + strideTurnPart,
                             -STRIDE_TURN, STRIDE_TURN);

    float phaseOff = gaitRev ? legs[PHASE_MIRROR[i]].phaseOffset : L.phaseOffset;
    float q = fracf(p - phaseOff);
    float kneeOff = BODY_DROP;
    if (L.frontSign < 0) kneeOff -= BACK_KNEE_TILT;   // back feet stand straighter
    float u;   // where the foot is along its stride: +0.5 front, -0.5 back

    if (q < SWING_DUTY) {
      // swing: foot in the air, hip returns from back to front. The hip runs on
      // a smoothstep so it is nearly stopped at both ends of the swing — the
      // foot sets down with no forward speed instead of being dragged on.
      float w  = q / SWING_DUTY;
      float ws = w * w * (3.0f - 2.0f * w);
      u = -0.5f + ws;
      kneeOff += L.kneeLift * liftProfile(w);
    } else {
      // stance: foot planted, sweeping front to back — this is propulsion. The
      // sweep now runs at a constant rate the whole way: the old settle step
      // held this hip still for ~90ms while the other two stance feet kept
      // pushing, so the leg that had just landed was dragged every step.
      float s = (q - SWING_DUTY) / (1.0f - SWING_DUTY);
      u = 0.5f - s;
      if (s < STANCE_SETTLE) kneeOff -= FOOT_PRESS * (1.0f - s / STANCE_SETTLE);
    }

    // The straight-line correction. u is shared with the hip, so the swing foot
    // retraces the same path back through the air and lands at the reach it
    // needs — no step in the knee command at touchdown.
    kneeOff += reachRatioCmd * strideFwdPart * u * L.frontSign;

    out[L.kneeCh] = POSE_STAND[L.kneeCh] + L.kneeSign * kneeOff;
    out[L.hipCh]  = POSE_STAND[L.hipCh]  + L.hipSign  * (stride * u);
  }
}

// Interpolate from wherever the servos are now to a target pose.
void moveTo(const float target[8], int ms) {
  float start[8];
  memcpy(start, angles, sizeof(start));
  int steps = max(1, ms / UPDATE_MS);
  for (int s = 1; s <= steps; s++) {
    float t = (float)s / steps;
    for (int i = 0; i < 8; i++) setServo(i, start[i] + (target[i] - start[i]) * t);
    delay(UPDATE_MS);
  }
}

// ---------- emotes ----------

void emoteWave() {
  float stand[8], prep[8], up[8], down[8];
  poseStand(stand);
  memcpy(prep, stand, sizeof(stand));
  memcpy(up,   stand, sizeof(stand));
  memcpy(down, stand, sizeof(stand));

  prep[BL_KNEE] = -47;                          // counterbalance before lifting
  up[BL_KNEE]   = -47;  up[FR_KNEE]   = -69;    // front-right arm high
  down[BL_KNEE] = -47;  down[FR_KNEE] = -26;    // and back down

  moveTo(prep, 450);
  for (int i = 0; i < 3; i++) { moveTo(up, 320); moveTo(down, 320); }
  moveTo(prep, 400);
  moveTo(stand, 500);
}

// Standing side-to-side wiggle — knees ripple front pair against back pair.
void emoteShimmy() {
  const float amp      = 20;
  const int   periodMs = 1400;
  const int   cycles   = 3;
  float phase[4];
  phase[LEG_FL] = 0.0f;  phase[LEG_FR] = 0.0f;
  phase[LEG_BL] = 0.5f;  phase[LEG_BR] = 0.5f;

  float pose[8];
  poseStand(pose);
  moveTo(pose, 300);

  unsigned long start = millis();
  unsigned long total = (unsigned long)periodMs * cycles;
  while (millis() - start < total) {
    float p = (float)(millis() - start) / periodMs;
    for (int i = 0; i < 4; i++) {
      LegConfig &L = legs[i];
      float off = BODY_DROP + amp * sinf(2 * PI * (p - phase[i]));
      pose[L.kneeCh] = POSE_STAND[L.kneeCh] + L.kneeSign * off;
      pose[L.hipCh]  = POSE_STAND[L.hipCh];
    }
    setPose(pose);
    delay(UPDATE_MS);
  }

  poseStand(pose);
  moveTo(pose, 400);
}

// Drop flat to the floor, then undulate front-to-back like a worm.
void emoteWorm() {
  const float amp      = 26;
  const int   periodMs = 1200;
  const int   cycles   = 4;
  float phase[4];
  phase[LEG_FL] = 0.0f;  phase[LEG_FR] = 0.0f;
  phase[LEG_BL] = 0.5f;  phase[LEG_BR] = 0.5f;

  float pose[8];
  memcpy(pose, POSE_FLAT, sizeof(POSE_FLAT));
  moveTo(pose, 1000);              // ease down, don't drop

  unsigned long start = millis();
  unsigned long total = (unsigned long)periodMs * cycles;
  while (millis() - start < total) {
    float p = (float)(millis() - start) / periodMs;
    for (int i = 0; i < 4; i++) {
      LegConfig &L = legs[i];
      pose[L.kneeCh] = POSE_FLAT[L.kneeCh] + L.kneeSign * amp * sinf(2 * PI * (p - phase[i]));
      pose[L.hipCh]  = POSE_FLAT[L.hipCh];
    }
    setPose(pose);
    delay(UPDATE_MS);
  }

  memcpy(pose, POSE_FLAT, sizeof(POSE_FLAT));
  moveTo(pose, 400);
  poseStand(pose);
  moveTo(pose, 1200);              // stand back up slowly
}

// ------------------------------------------

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Gordo</title>
<style>
  *{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
  body{font-family:monospace;background:#111;color:#eee;margin:0;padding:16px;
       max-width:520px;margin:0 auto;touch-action:none;user-select:none}
  h1{color:#4af;margin:0 0 2px;font-size:20px}
  .sub{color:#666;font-size:11px;margin-bottom:18px}
  .stick-wrap{display:flex;justify-content:center;margin-bottom:8px}
  #stick{position:relative;width:240px;height:240px;border-radius:50%;
         background:radial-gradient(circle,#1c1c1c 0%,#161616 70%,#141414 100%);
         border:2px solid #333;touch-action:none}
  #knob{position:absolute;width:84px;height:84px;border-radius:50%;
        background:#4af;border:2px solid #7cf;left:50%;top:50%;
        transform:translate(-50%,-50%);box-shadow:0 0 18px rgba(68,170,255,.35)}
  .axis{position:absolute;color:#3a3a3a;font-size:11px}
  .axis.n{top:8px;left:50%;transform:translateX(-50%)}
  .axis.s{bottom:8px;left:50%;transform:translateX(-50%)}
  .axis.w{left:10px;top:50%;transform:translateY(-50%)}
  .axis.e{right:10px;top:50%;transform:translateY(-50%)}
  .readout{text-align:center;color:#666;font-size:11px;height:16px;margin-bottom:14px}
  .row{display:flex;gap:12px;margin-bottom:12px}
  button{flex:1;font-family:monospace;font-size:14px;padding:16px 8px;border-radius:10px;
         cursor:pointer;background:#1e1e1e;border:1px solid #444;color:#ccc}
  button:active,button.on{background:#123;border-color:#4af;color:#4af}
  .emote{border-color:#4a4;color:#4a4}
  .emote:active{background:#1e3a1e}
  button:disabled{opacity:.35}
  .stop{border-color:#e55;color:#e55}
  .stop:active{background:#3a1e1e}
  h2{color:#666;font-size:11px;text-transform:uppercase;letter-spacing:1px;margin:18px 0 8px}
</style>
</head>
<body>
<h1>Gordo</h1>
<div class="sub">drag the stick &mdash; release to stop</div>

<div class="stick-wrap">
  <div id="stick">
    <span class="axis n">FWD</span><span class="axis s">BACK</span>
    <span class="axis w">LEFT</span><span class="axis e">RIGHT</span>
    <div id="knob"></div>
  </div>
</div>
<div class="readout" id="readout"></div>

<h2>Straight trim</h2>
<div class="row">
  <button onclick="trim(-0.02)">&#9664; more left</button>
  <button id="trimVal" onclick="trim(0)">0.00</button>
  <button onclick="trim(0.02)">more right &#9654;</button>
</div>

<h2>Foot lift</h2>
<div class="row">
  <button onclick="lift(-3)">&#9660; lower</button>
  <button id="liftVal" onclick="lift(0)">--</button>
  <button onclick="lift(3)">higher &#9650;</button>
</div>

<h2>Reach (straight-line stance)</h2>
<div class="row">
  <button onclick="tune('reach',-0.1)">&#9664; less</button>
  <button id="reachVal" onclick="tune('reach',0)">--</button>
  <button onclick="tune('reach',0.1)">more &#9654;</button>
</div>

<h2>Stride</h2>
<div class="row">
  <button onclick="tune('stride',-2)">&#9664; shorter</button>
  <button id="strideVal" onclick="tune('stride',0)">--</button>
  <button onclick="tune('stride',2)">longer &#9654;</button>
</div>

<h2>Emotes</h2>
<div class="row">
  <button class="emote" onclick="emote('wave',5200)">&#128075; Wave</button>
  <button class="emote" onclick="emote('shimmy',5400)">&#127881; Shimmy</button>
  <button class="emote" onclick="emote('worm',8200)">&#127807; Worm</button>
</div>
<div class="row">
  <button class="stop" onclick="allStop()">Stop / Stand</button>
</div>

<script>
const stick=document.getElementById('stick');
const knob=document.getElementById('knob');
const readout=document.getElementById('readout');
const R=78;               // max knob travel from center
let x=0,y=0,sent={x:0,y:0},dragging=false,busy=false;

function place(px,py){
  knob.style.left=(120+px)+'px';
  knob.style.top=(120+py)+'px';
}

function setVec(nx,ny){
  x=nx; y=ny;
  place(x*R, -y*R);
  readout.innerText = (x||y) ? `fwd ${y.toFixed(2)}  turn ${x.toFixed(2)}` : '';
}

function fromEvent(e){
  const r=stick.getBoundingClientRect();
  const t=e.touches?e.touches[0]:e;
  let dx=(t.clientX-r.left-r.width/2)/R;
  let dy=-(t.clientY-r.top-r.height/2)/R;
  const m=Math.hypot(dx,dy);
  if(m>1){ dx/=m; dy/=m; }
  if(Math.abs(dx)<0.12) dx=0;    // deadzone
  if(Math.abs(dy)<0.12) dy=0;
  setVec(dx,dy);
}

function startDrag(e){ if(busy) return; dragging=true; fromEvent(e); e.preventDefault(); }
function moveDrag(e){ if(dragging){ fromEvent(e); e.preventDefault(); } }
function endDrag(){ if(!dragging) return; dragging=false; setVec(0,0); push(true); }

stick.addEventListener('mousedown',startDrag);
stick.addEventListener('touchstart',startDrag,{passive:false});
window.addEventListener('mousemove',moveDrag);
window.addEventListener('touchmove',moveDrag,{passive:false});
window.addEventListener('mouseup',endDrag);
window.addEventListener('touchend',endDrag);
window.addEventListener('touchcancel',endDrag);

function push(force){
  if(busy) return;
  if(force || x!==sent.x || y!==sent.y || x || y){
    sent={x,y};
    fetch(`/drive?x=${x.toFixed(2)}&y=${y.toFixed(2)}`).catch(()=>{});
  }
}
setInterval(push,120);   // also acts as the device-side keepalive

function emote(name,ms){
  if(busy) return;
  busy=true;
  setVec(0,0);
  document.querySelectorAll('button').forEach(b=>b.disabled=true);
  readout.innerText=name+'...';
  fetch('/emote?name='+name).catch(()=>{});
  setTimeout(()=>{
    busy=false;
    document.querySelectorAll('button').forEach(b=>b.disabled=false);
    readout.innerText='';
  },ms);
}

function allStop(){ setVec(0,0); fetch('/stop').catch(()=>{}); }

function trim(delta){
  fetch('/trim?d='+delta).then(r=>r.text()).then(v=>{
    document.getElementById('trimVal').innerText=(+v).toFixed(2);
  }).catch(()=>{});
}
function lift(delta){
  fetch('/lift?d='+delta).then(r=>r.text()).then(v=>{
    document.getElementById('liftVal').innerText=(+v).toFixed(0)+'\u00b0';
  }).catch(()=>{});
}

function tune(name,delta){
  fetch('/'+name+'?d='+delta).then(r=>r.text()).then(v=>{
    document.getElementById(name+'Val').innerText=v;
  }).catch(()=>{});
}

trim(0);   // read the saved values on load
lift(0);
tune('reach',0);
tune('stride',0);
</script>
</body>
</html>
)rawliteral";

void handleRoot() { server.send_P(200, "text/html", INDEX_HTML); }

void handleDrive() {
  float x = constrain(server.arg("x").toFloat(), -1.0f, 1.0f);
  float y = constrain(server.arg("y").toFloat(), -1.0f, 1.0f);
  lastDriveMs = millis();

  if (mode != MODE_EMOTE) {
    driveX = x;
    driveY = y;
    if (x == 0 && y == 0) {
      if (mode == MODE_DRIVE) stopping = true;   // finish the cycle, then stand
    } else if (mode == MODE_IDLE) {
      mode      = MODE_DRIVE;
      gaitPhase = 0;
      gaitRev   = (y < 0);
      stopping  = false;
    } else {
      stopping = false;
    }
  }
  server.send(200, "text/plain", "ok");
}

void handleEmote() {
  String name = server.arg("name");
  if (mode != MODE_EMOTE) {
    pendingEmote = (name == "wave") ? 1 : (name == "shimmy") ? 2 : (name == "worm") ? 3 : 0;
    if (pendingEmote) mode = MODE_EMOTE;
  }
  server.send(200, "text/plain", "ok");
}

void handleTrim() {
  float d = server.arg("d").toFloat();
  if (d != 0) {
    driveTrim = constrain(driveTrim + d, -0.5f, 0.5f);
    prefs.putFloat("trim", driveTrim);
    Serial.printf("driveTrim = %.2f\n", driveTrim);
  }
  server.send(200, "text/plain", String(driveTrim, 2));
}

// Live foot-lift adjust. Writes through to every leg so the change lands on the
// very next swing, and to flash so it survives a power cycle.
void handleLift() {
  float d = server.arg("d").toFloat();
  if (d != 0) {
    kneeLiftCmd = constrain(kneeLiftCmd + d, 0.0f, 75.0f);
    for (int i = 0; i < 4; i++) legs[i].kneeLift = kneeLiftCmd;
    prefs.putFloat("lift", kneeLiftCmd);
    Serial.printf("kneeLift = %.0f\n", kneeLiftCmd);
  }
  server.send(200, "text/plain", String(kneeLiftCmd, 0));
}

void handleReach() {
  float d = server.arg("d").toFloat();
  if (d != 0) {
    reachRatioCmd = constrain(reachRatioCmd + d, 0.0f, 3.0f);
    prefs.putFloat("reach", reachRatioCmd);
    Serial.printf("reachRatio = %.1f\n", reachRatioCmd);
  }
  server.send(200, "text/plain", String(reachRatioCmd, 1));
}

void handleStride() {
  float d = server.arg("d").toFloat();
  if (d != 0) {
    strideFwdCmd = constrain(strideFwdCmd + d, 6.0f, 55.0f);
    prefs.putFloat("stride", strideFwdCmd);
    Serial.printf("strideFwd = %.0f\n", strideFwdCmd);
  }
  server.send(200, "text/plain", String(strideFwdCmd, 0));
}

void handleStopSilent() {
  mode     = MODE_IDLE;
  stopping = false;
  driveX = driveY = 0;
  float p[8];
  poseStand(p);
  setPose(p);
}

void handleStop() {
  mode     = MODE_IDLE;
  stopping = false;
  driveX = driveY = 0;
  float p[8];
  poseStand(p);
  setPose(p);
  server.send(200, "text/plain", "ok");
}

void setup() {
  Serial.begin(115200);
  prefs.begin("gordo", false);
  driveTrim   = prefs.getFloat("trim", 0.0f);
  kneeLiftCmd   = prefs.getFloat("lift",   KNEE_LIFT);
  reachRatioCmd = prefs.getFloat("reach",  REACH_RATIO);
  strideFwdCmd  = prefs.getFloat("stride", STRIDE_FWD);
  Serial.printf("driveTrim = %.2f  kneeLift = %.0f  reach = %.1f  stride = %.0f\n",
                driveTrim, kneeLiftCmd, reachRatioCmd, strideFwdCmd);
  Wire.begin(8, 9);
  Wire.setClock(100000);
  pca.begin();
  pca.setPWMFreq(50);

  // swing order FR -> BL -> FL -> BR keeps the CoM inside the support triangle
  legs[LEG_FL] = LegConfig{FL_KNEE, FL_HIP, +1, -1, +1, 0.50f, kneeLiftCmd, +1};
  legs[LEG_FR] = LegConfig{FR_KNEE, FR_HIP, -1, +1, -1, 0.00f, kneeLiftCmd, +1};
  legs[LEG_BR] = LegConfig{BR_KNEE, BR_HIP, -1, +1, -1, 0.75f, kneeLiftCmd, -1};
  legs[LEG_BL] = LegConfig{BL_KNEE, BL_HIP, +1, -1, +1, 0.25f, kneeLiftCmd, -1};

  setPose(POSE_STAND);

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("Connect to WiFi: %s  pass: %s\n", AP_SSID, AP_PASS);
  Serial.printf("Then open: http://%s\n", WiFi.softAPIP().toString().c_str());

  server.on("/", handleRoot);
  server.on("/drive", handleDrive);
  server.on("/emote", handleEmote);
  server.on("/trim", handleTrim);
  server.on("/lift", handleLift);
  server.on("/reach", handleReach);
  server.on("/stride", handleStride);
  server.on("/stop", handleStop);
  server.begin();

  float p[8];
  poseStand(p);
  moveTo(p, 500);
}

void loop() {
  server.handleClient();

  if (mode == MODE_EMOTE) {
    if (pendingEmote == 1) emoteWave();
    else if (pendingEmote == 2) emoteShimmy();
    else if (pendingEmote == 3) emoteWorm();
    pendingEmote = 0;
    mode = MODE_IDLE;
    return;
  }

  if (mode != MODE_DRIVE) return;

  // Dead-man switch: if the controller stops sending, stop walking.
  if (millis() - lastDriveMs > DRIVE_TIMEOUT) { handleStopSilent(); return; }

  unsigned long now = millis();
  if (now - lastUpdate < UPDATE_MS) return;
  float dt = (float)(now - lastUpdate);
  lastUpdate = now;

  float prev = gaitPhase;
  gaitPhase  = fracf(gaitPhase + dt / CYCLE_MS);

  // Stop only at a cycle boundary so the robot finishes with all feet planted.
  if (stopping && gaitPhase < prev) { handleStopSilent(); return; }

  // Same boundary is the only safe place to flip direction: moving a leg to a
  // different slot mid-swing would teleport it across the sequence.
  if (gaitPhase < prev) gaitRev = (driveY < 0);

  float pose[8];
  computeDrivePose(gaitPhase, driveY, driveX, pose);
  setPose(pose);
}
