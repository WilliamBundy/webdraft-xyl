import struct
import json
import sys

data = None

filename = sys.argv[1]
basename = filename.rpartition(".")[0]


with open(filename, 'r', encoding='utf-8') as jfile:

	data = json.load(jfile)

mons = data['species']
moves = data['moves']
abilities = data['abilities']

mon_names = sorted(list(mons.keys()), key=lambda x: mons[x]['number'])
move_names = sorted(list(moves.keys()), key=lambda x: moves[x]['index'])
ability_names = sorted(list(abilities.keys()), key=lambda x: abilities[x]['index'])

move_lookup = {}
for i,name in enumerate(move_names):
	move_lookup[name] = i

""" key_moves = [
"SPIKES", "RAPID_SPIN", "EXPLOSION", "SELFDESTRUCT", "BATON_PASS",
"HEAL_BELL", "PURSUIT", "LIGHT_SCREEN", "REFLECT", "ROAR", "WHIRLWIND", 
"THUNDER_WAVE", "STUN_SPORE", "SING", "HYPNOSIS", "SLEEP_POWDER", "SPORE",
"LOVELY_KISS", "RECOVER", "MOONLIGHT", "MORNING_SUN", "SYNTHESIS", "GLARE"]

#for mv in key_moves:
#	print("\tMV_" + mv + ',')
"""


headerstr = struct.Struct("IIII")
movestr = struct.Struct("16s16s64sBBbBBBBi")
monstr = struct.Struct("16siiii2B6B100H")
abilitystr = struct.Struct("16s32s")

mondata = bytearray()
movedata = bytearray()
abilitydata = bytearray()

mondata.resize(headerstr.size + monstr.size * len(mons))
movedata.resize(headerstr.size + movestr.size * len(moves))
abilitydata.resize(headerstr.size + abilitystr.size * len(abilities))

typenums = {}
	
typestring = """Type_Normal,
Type_Fighting,
Type_Flying,
Type_Poison,
Type_Ground,
Type_Rock,
Type_Bug,
Type_Ghost,
Type_Steel,
Type_Fire,
Type_Water,
Type_Grass,
Type_Electric,
Type_Psychic,
Type_Ice,
Type_Dragon,
Type_Dark,
Type_Fairy"""
for i,line in enumerate(typestring.split("\n")):
	name = (line.strip(',').partition('_')[-1])
	typenums[name] = i


#print(typenums)

def fourcc(s):
	c = 0
	c |= ord(s[0])
	c |= ord(s[1]) << 8
	c |= ord(s[2]) << 16
	c |= ord(s[3]) << 24
	return c

Table_Mons = 1
Table_Moves = 2
Table_Points = 3
Table_Abilities = 6

headerstr.pack_into(mondata, 0, 
	fourcc('TABL'),
	0x00010000,
	Table_Mons,
	len(mons))

namefix = {
	"MR  MIME": "MR. MIME",
	"FARFETCH D": "FARFETCH'D",
	"NIDORAN F": "NIDORAN-F",
	"NIDORAN M": "NIDORAN-M"
}

def fixName(name):
	name = name.replace('_', ' ')
	if name in namefix:
		name = namefix[name]
	name = name.title()
	return name.encode('utf-8')


for i in range(len(mons)):
	offset = headerstr.size + monstr.size * i
	mon = mons[mon_names[i]]

	types = [
		typenums[mon['types'][0]],
		typenums[mon['types'][1]] if len(mon['types']) == 2 else typenums[mon['types'][0]]
	]

	#print(mon['types'])

	mv = list(map(lambda name: move_lookup[name], mon['moves']))
	mv.extend([0xFFFF,] * (100-len(mv)))
	preevo = mon['preevolution'] if 'preevolution' in mon else ''

	abis = mon['abilities']
	abi1 = abilities[abis[0]]['index']
	abi2 = -1
	if len(abis) > 1:
		abi2 = abilities[abis[1]]['index']


	

	monstr.pack_into(mondata, offset, 
		fixName(mon['name']),
		(mons[mon['preevolution']]['number'] - 1) if preevo != '' else -1,
		mon['number'],
		abi1, abi2,
		*types,
		*mon['stats'],
		*mv)

with open(basename + ".wdmons", "wb") as ff:
	ff.write(mondata)

headerstr.pack_into(movedata, 0, 
	fourcc('TABL'),
	0x00010000,
	Table_Moves,
	len(moves))

for i in range(len(moves)):
	offset = headerstr.size + movestr.size * i
	move = moves[move_names[i]]
	#print(move['name'], move['index'], move['type'])
	movestr.pack_into(movedata, offset,
		move['name'].replace('_', ' ').title().encode('utf-8'),
		move['effect'].replace('_', ' ').title().encode('utf-8'),
		move['description'].encode('utf-8'),
		move['accuracy'],
		move['power'],
		move['priority'],
		typenums[move['type']],
		move['secondaryChance'],
		move['pp'],
		move['target_val'],
		move['index'])

with open(basename + ".wdmoves", "wb") as ff:
	ff.write(movedata)

headerstr.pack_into(abilitydata, 0, 
	fourcc('TABL'),
	0x00010000,
	Table_Abilities,
	len(abilities))
for i in range(len(abilities)):
	abi = abilities[ability_names[i]]
	#print(abi['name'], len(abi['name']))
	#print(abi['description'], len(abi['description']))

	offset = headerstr.size + abilitystr.size * i
	abilitystr.pack_into(abilitydata, offset,
		abi['name'].title().encode('utf-8'),
		abi['description'].encode('utf-8'))

with open(basename + ".wdabi", "wb") as ff:
	ff.write(abilitydata)
