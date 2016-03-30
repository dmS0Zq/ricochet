#!/usr/bin/env python3
import random

random.seed()

players = [
    { 'name': 'a', 'id': random.getrandbits(1024), 'seed': 0, 'pos': 0 },
    { 'name': 'b', 'id': random.getrandbits(1024), 'seed': 0, 'pos': 0 },
    { 'name': 'c', 'id': random.getrandbits(1024), 'seed': 0, 'pos': 0 },
    { 'name': 'd', 'id': random.getrandbits(1024), 'seed': 0, 'pos': 0 },
    { 'name': 'e', 'id': random.getrandbits(1024), 'seed': 0, 'pos': 0 },
]

bad_actors = ['a','b','c','d']
dict_count = {}
st = ""

for i in range(0, 15000):
    for player in players:
        if not player['name'] in bad_actors:
            player['seed'] = random.getrandbits(1024)
            # if id does not change (ie, it is the onion service
            # public key), then only a subset of permutations 
            # ever get used
            #player['id'] = random.getrandbits(1024)
    combined_seed = 0
    for player in players:
        combined_seed = combined_seed ^ player['seed']
    #print("("+bin(combined_seed)[2:]+")",end='\t')
    for player in players:
        player['pos'] = player['id'] ^ combined_seed 
    #    print(bin(player['pos'])[2:], end='\t')
    #print()
    new_players = sorted(players, key=lambda k: k['pos'])
    for p in new_players:
        st += p['name']
    if not st in dict_count:
        #print("first", st)
        dict_count[st] = 1
    else:
        dict_count[st] += 1
    st = ""
for k, v in dict_count.items():
    print(v,k)


