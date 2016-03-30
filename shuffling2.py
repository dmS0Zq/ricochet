#!/usr/bin/env python3
import random

random.seed()
bits = 1024

players = [
    # feel free to mess with the seeds 
    { 'name': 'a', 'seeds': [0,0,0,0,0,0,0], 'pos': 0 },
    { 'name': 'b', 'seeds': [0,0,0,0,0,0,0], 'pos': 0 },
    { 'name': 'c', 'seeds': [], 'pos': 0 },
    { 'name': 'd', 'seeds': [], 'pos': 0 },
    { 'name': 'e', 'seeds': [], 'pos': 0 },
    { 'name': 'f', 'seeds': [], 'pos': 0 },
    { 'name': 'g', 'seeds': [], 'pos': 0 },
]

# players who try to game the system by
# - not changing their seeds
# - illegally share their seeds with each other
# - or otherwise do bad things
# the algo works as long at least 1 player is not malicious
bad_actors = ['a', 'b']
combos = {}
string = ""

for _ in range(0, 150000):
    # initialize each player's seeds unless they are a bad actor
    for p in players:
        if not p['name'] in bad_actors or len(p['seeds'])!=len(players):
            p['seeds'] = []
            for __ in range(0, len(players)):
                p['seeds'].append(random.getrandbits(bits))
    # for every i'th player, XOR together every player's i'th seed into the
    # i'th player's position
    for i in range(0, len(players)):
        players[i]['pos'] = 0
        for p in players:
            players[i]['pos'] ^= p['seeds'][i]
    # sort the players into sp based on position
    sp = sorted(players, key=lambda k: k['pos'])
    # build up the string which shows the order of the players
    for s in sp:
        string += s['name']
    # add it to the combos or increment if already in combos
    if not string in combos:
        combos[string] = 1
    else:
        combos[string] += 1
    string = ""
for k, v in combos.items():
    print(v,k)
