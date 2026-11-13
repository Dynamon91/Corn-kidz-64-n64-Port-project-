# Phase 17 — original PlayerCtrl.Run state-0 jump and movement choice

**PROJECT IN DEVELOPMENT.** This source-grounded partial implementation is not a playable Corn Kidz 64 port. Original PC assembly stays private; the code in this repo is independently written C11.

Evidence from v1.029 PlayerCtrl.Run (ECMA-335 token 0x06000294): state dispatch at IL 7254, state 0 at IL 7504. Initial Gravity(0.02,0.6) and four MovPl call parameter choices: grounded slow zMov<0.6; faster grounded; air with addVel magnitude <0.1; air with >=0.1. Movement is *not executed* by this module.

State-0 jump from original instruction positions: A down while grounded or onFloorTimer>-4, provided slipTimer==0, sets yVel=.23, increments jumpNum, requests jump animation/SFX and StepParticles(8), and leaves floor. Early repeat jump with 0<onFloorTimer<11 and jumpNum>=2 clears jumpNum; at zMov>0.6 selects jumpspin and yVel=.24. Outside that branch, zMov>0.6 selects jumpF. A release while yVel>.15 and flip/fliploop not playing sets yVel=.1. X interaction on floor with interactable nearby object takes priority over A, sets state 5 and yVel=0.

Nine C11 test groups exercise ground/air, gravity, jump, coyote frame, slip, spin, short hop, interaction priority and atomic invalid-input rejection. Unity numerical parity, animation, physical state transitions and gameplay-level equivalence are not verified.

Optional N64 diagnostic: press L to show **source jump probe**; pressing A makes a synthetic one-shot state sample for the new source-derived jump code and shows yVel/event bits. This does NOT replace old wireframe collision/movement; test in ares and send a video. No original character/level assets or sounds included.

**Progress 17/50 = 34% of provisional milestone roadmap, NOT 34% of original game completed.**
