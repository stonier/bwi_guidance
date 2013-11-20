h(eql(visited(P),false),0). 
h(eql(served(P),false),0). 
h(eql(open(D),false),0).

:- v_approach(D,V_astep-1), not callforopen(D,V_astep).
:- v_callforopen(D,V_astep-1), not gothrough(D,V_astep).
:- v_gothrough(D,V_astep-1), approach(D,V_astep).

-noop(V_step) :- approach(D,V_step).
-noop(V_step) :- gothrough(D,V_step).
-noop(V_step) :- callforopen(D,V_step).
-noop(V_step) :- sense(P1,ploc(P2),V_step).
-noop(V_step) :- goto(P,V_step).

noop(V_step) :- not -noop(V_step).

v_approach(D,V_astep) :- approach(D,V_astep).
v_approach(D,V_astep) :- v_approach(D,V_astep-1), noop(V_astep).
v_callforopen(D,V_astep) :- callforopen(D,V_astep).
v_callforopen(D,V_astep) :- v_callforopen(D,V_astep-1), noop(V_astep).
v_gothrough(D,V_astep) :- gothrough(D,V_astep).
v_gothrough(D,V_astep) :- v_gothrough(D,V_astep-1), noop(V_astep).


%:- approach(Y,V_step),at(X,V_step), acc(X,Y,Z), 1{hasdoor(Z,N)}1, inside(peterstone,W,V_step), Z!=W.


%goal: serve everyone to collect their mail
goal(0,V_step) :- h(eql(served(alice),true),V_step).
goal(1,V_step) :- h(eql(served(bob),true),V_step).
goal(2,V_step) :- h(eql(served(carol),true),V_step).
goal(3,V_step) :- h(eql(served(dan),true),V_step).

goal(G,V_astep) :- goal(G,V_astep-1).

:- not goal(1,maxstep).
:- not goal(2,maxstep).
:- not goal(3,maxstep).
:- not goal(0,maxstep).

served(alice,V_step) :-goal(0,V_step).
served(bob,V_step) :-goal(1,V_step).
served(carol,V_step) :-goal(2,V_step).
served(dan,V_step) :-goal(3,V_step).

true.
:- false.
goal(2,17).goal(3,17).