import json
import sys

# get list of pokemon from file

mon_in_dex = []
with open(sys.argv[1]) as ff:
	for line in ff:
		mon_in_dex.append(line.strip())

#print(mon_in_dex)

dex_list = None
with open(sys.argv[2], encoding='utf-8') as ff:
	dex_list = json.load(ff)

pokedex = {}
for mon in dex_list:
	pokedex[mon['name']['english']] = mon


output = {'species':{}, 
	'moves':{
		"None": {
				"name": "None",
				"index": 0,
				"effect": "NO_EFFECT",
				"description": "No move data available",
				"power": 0,
				"type": "Normal",
				"accuracy": 100,
				"pp": 5,
				"secondaryChance": 0,
				"priority": 0,
				"target_val": 8,
				"flags_val": 22,
				"target": ["TARGET_RANDOM"],
				"flags": ["PROTECT_AFFECTED"]
		}
	}, 
	'abilities': {
		"None": {
			"name": "None",
			"description": "No data",
			"index": 0
		}
	}
}

for index, name in enumerate(mon_in_dex):
	mon = pokedex[name]
	b = mon['base']
	stats = [b['HP'], b['Attack'], b['Defense'], b['Sp. Attack'], b['Sp. Defense'], b['Speed']]
	newmon = {
		'name': name,
		'number': index,
		'preevolution': '',
		'types': mon['type'],
		'stats': stats,
		'moves': ['None'],
		'abilities': ['None']
	}
	output['species'][name] = newmon

with open('xy.json', 'w', encoding='utf-8') as ff:
	json.dump(output, ff, indent=2)



import os
import math
from PIL import Image

width = int(math.sqrt(len(mon_in_dex)) * 64 + 255) & ~255
print("spritesheet width:", width)

sheet = Image.new("RGBA", (width, width), (1, 1, 1, 1))


x = 0
y = 0
for index, name in enumerate(mon_in_dex):
	mon = pokedex[name]
	dexnum = mon['id']
	img = Image.open(os.path.join(sys.argv[3], "{:03d}MS.png".format(dexnum))).convert("RGBA")
	w = img.width
	dx = (64 - w) // 2
	dy = (64 - w) // 2
	sheet.alpha_composite(img, (x+dx, y+dy), (0, 0, img.width, img.width))
	x += 64
	if x >= sheet.width-1:
		x = 0
		y += 64
	img.close()
sheet.save("xy-sheet.png")

