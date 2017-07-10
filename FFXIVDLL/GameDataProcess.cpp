#include <Windows.h>
#include <deque>
#include <vector>
#include <map>
#include <algorithm>
#include "resource.h"
#include "Languages.h"
#include "FFXIVDLL.h"
#include "GameDataProcess.h"
#include "MedianCalculator.h"
#include "DPSWindowController.h"
#include "DOTWindowController.h"
#include "ChatWindowController.h"
#include "ImGuiConfigWindow.h"
#include "OverlayRenderer.h"
#include "Hooks.h"
#include <psapi.h>

std::string debug_skillname(int skillid) {
	switch (skillid) {
		case 0x9: return "fast blade";
		case 0xb: return "savage blade";
		case 0x14: return "fight or flight";
		case 0xe: return "flash";
		case 0xf: return "riot blade";
		case 0x18: return "shield lob";
		case 0x10: return "shield bash";
		case 0x15: return "rage of halone";
		case 0x19: return "shield swipe";
		case 0x11: return "sentinel";
		case 0x13: return "tempered will";
		case 0x16: return "bulwark";
		case 0x17: return "circle of scorn";
		case 0x35: return "bootshine";
		case 0x36: return "true strike";
		case 0x38: return "snap punch";
		case 0x3b: return "internal release";
		case 0x3c: return "fists of earth";
		case 0x3d: return "twin snakes";
		case 0x3e: return "arm of the destroyer";
		case 0x42: return "demolish";
		case 0x49: return "fists of wind";
		case 0x40: return "steel peak";
		case 0x41: return "mantra";
		case 0x43: return "howling fist";
		case 0x45: return "perfect balance";
		case 0x1f: return "heavy swing";
		case 0x23: return "skull sunder";
		case 0x26: return "berserk";
		case 0x29: return "overpower";
		case 0x2e: return "tomahawk";
		case 0x25: return "maim";
		case 0x28: return "thrill of battle";
		case 0x2f: return "butcher's block";
		case 0x2a: return "storm's path";
		case 0x2b: return "holmgang";
		case 0x2c: return "vengeance";
		case 0x2d: return "storm's eye";
		case 0x4b: return "true thrust";
		case 0x4e: return "vorpal thrust";
		case 0x51: return "impulse drive";
		case 0x4f: return "heavy thrust";
		case 0x5a: return "piercing talon";
		case 0x53: return "life surge";
		case 0x54: return "full thrust";
		case 0x55: return "blood for blood";
		case 0x57: return "disembowel";
		case 0x58: return "chaos thrust";
		case 0x61: return "heavy shot";
		case 0x62: return "straight shot";
		case 0x65: return "raging strikes";
		case 0x64: return "venomous bite";
		case 0x67: return "misery's end";
		case 0x6e: return "bloodletter";
		case 0x70: return "repelling shot";
		case 0x6a: return "quick nock";
		case 0x71: return "windbite";
		case 0x6b: return "barrage";
		case 0x77: return "stone";
		case 0x78: return "cure";
		case 0x79: return "aero";
		case 0x7c: return "medica";
		case 0x7d: return "raise";
		case 0x86: return "fluid aura";
		case 0x7f: return "stone ii";
		case 0x80: return "repose";
		case 0x87: return "cure ii";
		case 0x84: return "aero ii";
		case 0x85: return "medica ii";
		case 0x8e: return "blizzard";
		case 0x8d: return "fire";
		case 0x95: return "transpose";
		case 0x90: return "thunder";
		case 0x91: return "sleep";
		case 0x92: return "blizzard ii";
		case 0x9c: return "scathe";
		case 0x93: return "fire ii";
		case 0x94: return "thunder ii";
		case 0x9d: return "manaward";
		case 0x98: return "fire iii";
		case 0x9b: return "aetherial manipulation";
		case 0x1c: return "shield oath";
		case 0x1a: return "sword oath";
		case 0x1b: return "cover";
		case 0x1d: return "spirits within";
		case 0x1e: return "hallowed ground";
		case 0xdd6: return "sheltron";
		case 0xdd2: return "goring blade";
		case 0xdd4: return "divine veil";
		case 0xdd5: return "clemency";
		case 0xdd3: return "royal authority";
		case 0x1cd6: return "intervention";
		case 0x1cd8: return "holy spirit";
		case 0x1cd7: return "requiescat";
		case 0x1cd9: return "passage of arms";
		case 0x46: return "rockbreaker";
		case 0x47: return "shoulder tackle";
		case 0x3f: return "fists of fire";
		case 0x48: return "one ilm punch";
		case 0x4a: return "dragon kick";
		case 0x10a6: return "form shift";
		case 0xdda: return "meditation";
		case 0xddb: return "the forbidden chakra";
		case 0xdd9: return "elixir field";
		case 0xdd8: return "purification";
		case 0xdd7: return "tornado kick";
		case 0x1ce2: return "riddle of earth";
		case 0x1eb8: return "earth tackle";
		case 0x1eb9: return "wind tackle";
		case 0x1eba: return "fire tackle";
		case 0x1ebc: return "riddle of wind";
		case 0x1ce3: return "riddle of fire";
		case 0x1ce4: return "brotherhood";
		case 0x30: return "defiance";
		case 0x31: return "inner beast";
		case 0x32: return "unchained";
		case 0x33: return "steel cyclone";
		case 0x34: return "infuriate";
		case 0xddc: return "deliverance";
		case 0xddd: return "fell cleave";
		case 0xddf: return "raw intuition";
		case 0xde0: return "equilibrium";
		case 0xdde: return "decimate";
		case 0x1cda: return "onslaught";
		case 0x1cdb: return "upheaval";
		case 0x1cdc: return "shake it off";
		case 0x1cdd: return "inner release";
		case 0x5c: return "jump";
		case 0x5e: return "elusive jump";
		case 0x56: return "doom spike";
		case 0x5f: return "spineshatter dive";
		case 0x60: return "dragonfire dive";
		case 0xde5: return "battle litany";
		case 0xde1: return "blood of the dragon";
		case 0xde2: return "fang and claw";
		case 0xde4: return "wheeling thrust";
		case 0xde3: return "geirskogul";
		case 0x1ce5: return "sonic thrust";
		case 0x1ce6: return "dragon sight";
		case 0x1ce7: return "mirage dive";
		case 0x1ce8: return "nastrond";
		case 0x72: return "mage's ballad";
		case 0x73: return "foe requiem";
		case 0x74: return "army's paeon";
		case 0x75: return "rain of death";
		case 0x76: return "battle voice";
		case 0xde7: return "the wanderer's minuet";
		case 0x228a: return "pitch perfect";
		case 0xde6: return "empyreal arrow";
		case 0xde8: return "iron jaws";
		case 0xde9: return "the warden's paean";
		case 0xdea: return "sidewinder";
		case 0x1ced: return "troubador";
		case 0x1cee: return "caustic bite";
		case 0x1cef: return "stormbite";
		case 0x1cf0: return "nature's minne";
		case 0x1cf1: return "refulgent arrow";
		case 0x88: return "presence of mind";
		case 0x89: return "regen";
		case 0x83: return "cure iii";
		case 0x8b: return "holy";
		case 0x8c: return "benediction";
		case 0xdf1: return "asylum";
		case 0xdf0: return "stone iii";
		case 0xdf3: return "assize";
		case 0xdf4: return "aero iii";
		case 0xdf2: return "tetragrammaton";
		case 0x1d06: return "thin air";
		case 0x1d07: return "stone iv";
		case 0x1d08: return "divine benison";
		case 0x1d09: return "plenary indulgence";
		case 0x9e: return "convert";
		case 0x9f: return "freeze";
		case 0x9a: return "blizzard iii";
		case 0x99: return "thunder iii";
		case 0xa2: return "flare";
		case 0xdf5: return "ley lines";
		case 0xdf6: return "sharpcast";
		case 0xdf7: return "enochian";
		case 0xdf8: return "blizzard iv";
		case 0xdf9: return "fire iv";
		case 0x1cfb: return "between the lines";
		case 0x1cfc: return "thunder iv";
		case 0x1cfd: return "triplecast";
		case 0x1cfe: return "foul";
		case 0xa3: return "ruin";
		case 0xa4: return "bio";
		case 0xa5: return "summon";
		case 0xbe: return "physick";
		case 0xa6: return "aetherflow";
		case 0xa7: return "energy drain";
		case 0xa8: return "miasma";
		case 0xaa: return "summon ii";
		case 0xad: return "resurrection";
		case 0xb2: return "bio ii";
		case 0xae: return "bane";
		case 0xac: return "ruin ii";
		case 0xb0: return "rouse";
		case 0xb3: return "shadow flare";
		case 0xb4: return "summon iii";
		case 0xb5: return "fester";
		case 0xb6: return "tri-bind";
		case 0xb8: return "enkindle";
		case 0xdfa: return "painflare";
		case 0xdfb: return "ruin iii";
		case 0xb7: return "spur";
		case 0xdfc: return "tri-disaster";
		case 0xdfd: return "dreadwyrm trance";
		case 0xdfe: return "deathflare";
		case 0x1d02: return "ruin iv";
		case 0x1cff: return "aetherpact";
		case 0x1d1a: return "devotion";
		case 0x1d00: return "bio iii";
		case 0x1d01: return "miasma iii";
		case 0x1d03: return "summon bahamut";
		case 0x1d04: return "wyrmweave";
		case 0x1d05: return "enkindle bahamut";
		case 0x1d19: return "akh morn";
		case 0xb9: return "adloquium";
		case 0xba: return "succor";
		case 0xbc: return "sacred soil";
		case 0xbd: return "lustrate";
		case 0xdff: return "indomitability";
		case 0xe00: return "broil";
		case 0xe01: return "deployment tactics";
		case 0xe02: return "emergency tactics";
		case 0xe03: return "dissipation";
		case 0x1d0a: return "Excogitation";
		case 0x1d0b: return "Broil II";
		case 0x1d0c: return "Chain Strategem";
		case 0x1d0d: return "Aetherpact ";
		case 0x1d0e: return "Fey Union";
		case 0x1ebd: return "Dissolve Union";
		case 0x8c0: return "spinning edge";
		case 0x8c1: return "shade shift";
		case 0x8c2: return "gust slash";
		case 0x8c5: return "hide";
		case 0x8c6: return "assassinate";
		case 0x8c7: return "throwing dagger";
		case 0x8c8: return "mug";
		case 0x8d2: return "trick attack";
		case 0x8cf: return "aeoilian edge";
		case 0x8d0: return "jugulate";
		case 0x8d1: return "shadow fang";
		case 0x8ce: return "death blossom";
		case 0x8d3: return "ten";
		case 0x8d4: return "ninjutsu";
		case 0x8d5: return "chi";
		case 0x8d6: return "shukuchi";
		case 0x8d7: return "jin";
		case 0x8d8: return "kassatsu";
		case 0xded: return "smoke screen";
		case 0xdeb: return "armor crush";
		case 0xdec: return "shadewalker";
		case 0xdef: return "duality";
		case 0xdee: return "dream within a dream";
		case 0x8d9: return "fuma shuriken";
		case 0x8da: return "katon";
		case 0x8db: return "raiton";
		case 0x8dc: return "hyoton";
		case 0x8dd: return "huton";
		case 0x8de: return "doton";
		case 0x8df: return "suiton";
		case 0x8e0: return "rabbit medium";
		case 0x1ce9: return "hellfrog medium";
		case 0x1cea: return "bhavacakra";
		case 0x1ceb: return "ten chi jin";
		case 0xb32: return "split shot";
		case 0x2290: return "heated split shot";
		case 0xb34: return "slug shot";
		case 0x2291: return "heated slug shot";
		case 0xb33: return "reload";
		case 0xb3b: return "heartbreak";
		case 0xb3c: return "reassemble";
		case 0xb48: return "blank";
		case 0xb36: return "spread shot";
		case 0xb3f: return "quick reload";
		case 0xb38: return "hot shot";
		case 0xb41: return "rapid fire";
		case 0xb39: return "clean shot";
		case 0x2292: return "heated clean shot";
		case 0xb3e: return "wildfire";
		case 0xb30: return "rook autoturret";
		case 0xd9f: return "turret retrieval";
		case 0xb31: return "bishop autoturret";
		case 0xb40: return "gauss barrel";
		case 0x2337: return "remove barrel";
		case 0xb3a: return "gauss round";
		case 0xb47: return "dismantle";
		case 0xb45: return "hypercharge";
		case 0xb4a: return "ricochet";
		case 0x1cf2: return "cooldown";
		case 0x1cf6: return "barrel stabilizer";
		case 0x1cf7: return "rook overdrive";
		case 0x1cf8: return "rook overload";
		case 0x1cf9: return "bishop overload";
		case 0x1cfa: return "flamethrower";
		case 0xe21: return "hard slash";
		case 0xe23: return "spinning slash";
		case 0xe25: return "unleash";
		case 0xe27: return "syphon strike";
		case 0xe28: return "unmend";
		case 0xe29: return "blood weapon";
		case 0xe2b: return "power slash";
		case 0xe2d: return "grit";
		case 0xe2c: return "darkside";
		case 0xe2f: return "blood price";
		case 0xe30: return "souleater";
		case 0xe31: return "dark passenger";
		case 0xe32: return "dark mind";
		case 0xe33: return "dark arts";
		case 0xe34: return "shadow wall";
		case 0xe36: return "living dead";
		case 0xe37: return "salted earth";
		case 0xe38: return "plunge";
		case 0xe39: return "abyssal drain";
		case 0xe3a: return "sole survivor";
		case 0xe3b: return "carve and spit";
		case 0x1cde: return "delirium";
		case 0x1cdf: return "quietus";
		case 0x1ce0: return "bloodspiller";
		case 0x1ce1: return "the blackest night";
		case 0xe0c: return "malefic";
		case 0xe0a: return "benefic";
		case 0xe0f: return "combust";
		case 0xe16: return "lightspeed";
		case 0xe10: return "helios";
		case 0xe13: return "ascend";
		case 0xe1e: return "essential dignity";
		case 0xe1a: return "benefic ii";
		case 0xe06: return "draw";
		case 0xe14: return "diurnal sect";
		case 0xe0b: return "aspected benefic";
		case 0xe07: return "royal road";
		case 0xe19: return "disable";
		case 0xe08: return "spread";
		case 0xe11: return "aspected helios";
		case 0xe09: return "redraw";
		case 0xe18: return "combust ii";
		case 0xe15: return "nocturnal sect";
		case 0xe1c: return "synastry";
		case 0xe1f: return "gravity";
		case 0xe0e: return "malefic ii";
		case 0xe1b: return "time dilation";
		case 0xe1d: return "collective unconscious";
		case 0xe20: return "celestial opposition";
		case 0x1d0f: return "earthly star";
		case 0x2084: return "stellar detonation";
		case 0x1d11: return "stellar explosion";
		case 0x1d12: return "malefic iii";
		case 0x1d13: return "minor arcana";
		case 0x1d18: return "sleeve draw";
		case 0x1131: return "the balance";
		case 0x1132: return "the bole";
		case 0x1135: return "the ewer";
		case 0x1136: return "the spire";
		case 0x1133: return "the arrow";
		case 0x1134: return "the spear";
		case 0x1d14: return "lord of crowns";
		case 0x1d15: return "lady of crowns";
		case 0x1d35: return "hakaze";
		case 0x1d36: return "jinpu";
		case 0x1d4a: return "third eye";
		case 0x1d4c: return "ageha";
		case 0x1d3e: return "enpi";
		case 0x1d37: return "shifu";
		case 0x1d3b: return "fuga";
		case 0x1d39: return "gekko";
		case 0x1ebb: return "iaijutsu";
		case 0x1d41: return "higanbana";
		case 0x1d40: return "tenka goken";
		case 0x1d3f: return "midare setsugekka";
		case 0x1d3c: return "mangetsu";
		case 0x1d3a: return "kasha";
		case 0x1d3d: return "oka";
		case 0x1d38: return "yukikaze";
		case 0x1d4b: return "meikyo shisui";
		case 0x1d46: return "hissatsu: kaiten";
		case 0x1d44: return "hissatsu: gyoten";
		case 0x1d43: return "hissatsu: yaten";
		case 0x1d4e: return "merciful eyes";
		case 0x1d49: return "meditate";
		case 0x1d42: return "hissatsu: shinten";
		case 0x1d4d: return "hissatsu: seigan";
		case 0x1d47: return "hagakure";
		case 0x1d48: return "hissatsu: guren";
		case 0x1d50: return "riposte";
		case 0x1d4f: return "jolt";
		case 0x1d51: return "verthunder";
		case 0x1d52: return "corps-a-corps";
		case 0x1d53: return "veraero";
		case 0x1d54: return "tether";
		case 0x1d55: return "scatter";
		case 0x1d56: return "verfire";
		case 0x1d57: return "verstone";
		case 0x1d58: return "zwerchhau";
		case 0x1d5b: return "displacement";
		case 0x1d5d: return "fleche";
		case 0x1d5c: return "redoublement";
		case 0x1d5e: return "acceleration";
		case 0x1d59: return "moulinet";
		case 0x1d5a: return "vercure";
		case 0x1d5f: return "contre sixte";
		case 0x1d60: return "embolden";
		case 0x1d61: return "manafication";
		case 0x1d64: return "jolt ii";
		case 0x1d63: return "verraise";
		case 0x1d62: return "impact";
		case 0x1d65: return "verflare";
		case 0x1d66: return "verholy";
		case 0x1d67: return "enchanted riposte";
		case 0x1d68: return "enchanted zwerchhau";
		case 0x1d69: return "enchanted redoublement";
		case 0x1d6a: return "enchanted moulinet";
		case 0x1d6b: return "rampart";
		case 0x1d74: return "low blow";
		case 0x1d6d: return "provoke";
		case 0x1d6c: return "convalescence";
		case 0x1d70: return "anticipation";
		case 0x1d6f: return "reprisal";
		case 0x1d6e: return "awareness";
		case 0x1d72: return "interject";
		case 0x1d73: return "ultimatum";
		case 0x1d71: return "shirk";
		case 0x1d8f: return "cleric stance";
		case 0x1d86: return "break";
		case 0x1d94: return "protect";
		case 0x1d90: return "esuna";
		case 0x1d8a: return "lucid dreaming";
		case 0x1d89: return "swiftcast";
		case 0x1d91: return "eye for an eye";
		case 0x1d92: return "largesse";
		case 0x1d87: return "surecast";
		case 0x1d93: return "rescue";
		case 0x1d75: return "second wind";
		case 0x1d7c: return "arm's length";
		case 0x1eb7: return "leg sweep";
		case 0x1d79: return "diversion";
		case 0x1d78: return "invigorate";
		case 0x1d76: return "bloodbath";
		case 0x1d77: return "goad";
		case 0x1d7d: return "feint";
		case 0x1d7b: return "crutch";
		case 0x1d7a: return "true north";
		case 0x1d81: return "foot graze";
		case 0x1d82: return "leg graze";
		case 0x1d85: return "peloton";
		case 0x1d83: return "tactician";
		case 0x1d84: return "refresh";
		case 0x1d7f: return "head graze";
		case 0x1d80: return "arm graze";
		case 0x1d7e: return "palisade";
		case 0x1d88: return "addle";
		case 0x1d8c: return "drain";
		case 0x1d8d: return "mana shift";
		case 0x1d8b: return "apocatastasis";
		case 0x1d8e: return "erase";
		case 0x27d: return "gust";
		case 0x27e: return "backdraft";
		case 0x27f: return "downburst";
		case 0x280: return "shining emerald";
		case 0x27a: return "shining topaz";
		case 0x279: return "gouge";
		case 0x27b: return "curl";
		case 0x27c: return "storm";
		case 0x318: return "wind blade";
		case 0x319: return "shockwave";
		case 0x31a: return "aerial slash";
		case 0x31b: return "contagion";
		case 0x31c: return "aerial blast";
		case 0x313: return "rock buster";
		case 0x314: return "mountain buster";
		case 0x315: return "earthen ward";
		case 0x316: return "landslide";
		case 0x317: return "earthen fury";
		case 0x338: return "choco slash";
		case 0x337: return "choco beak";
		case 0x336: return "choco rush";
		case 0x335: return "choco blast";
		case 0x334: return "choco drop";
		case 0x333: return "choco kick";
		case 0x332: return "choco guard";
		case 0x331: return "choco strike";
		case 0x339: return "choco medica";
		case 0x33a: return "choco surge";
		case 0x33b: return "choco cure";
		case 0x33c: return "choco regen";
		case 0x31d: return "crimson cyclone";
		case 0x31e: return "burning strike";
		case 0x31f: return "radiant shield";
		case 0x320: return "flaming crush";
		case 0x321: return "inferno";
		case 0x322: return "embrace";
		case 0x323: return "whispering dawn";
		case 0x324: return "fey covenant";
		case 0x325: return "fey illumination";
		case 0x326: return "embrace-selene";
		case 0x327: return "silent dusk";
		case 0x32b: return "fey wind";
		case 0x32a: return "fey caress";
		case 0xb4c: return "aether mortar";
		case 0xb4b: return "volley fire";
		case 0xe05: return "charged aether mortar";
		case 0xe04: return "charged volley fire";
		case 0x1d10: return "stellar burst";
		case 0xc8: return "braver";
		case 0xc9: return "bladedance";
		case 0xca: return "final heaven";
		case 0xcb: return "skyshard";
		case 0xcc: return "starstorm";
		case 0xcd: return "meteor";
		case 0xce: return "healing wind";
		case 0xcf: return "breath of the earth";
		case 0xd0: return "pulse of life";
		case 0xc5: return "shield wall";
		case 0xc6: return "mighty guard";
		case 0xc7: return "last bastion";
		case 0x1090: return "land walker";
		case 0x1091: return "dark force";
		case 0x1092: return "dragonsong dive";
		case 0x1093: return "chimatsuri";
		case 0x108f: return "desperado";
		case 0x108e: return "big shot";
		case 0x1095: return "satellite beam";
		case 0x1094: return "sagittarius arrow";
		case 0x1096: return "teraflare";
		case 0x1097: return "angel feathers";
		case 0x1098: return "astral stasis";
		case 0x1eb5: return "doom of the living";
		case 0x1eb6: return "vermillion scourge";
		case 0x2d4: return "digest";
		case 0x5cd: return "lunatic voice";
		case 0x67b: return "putrid cloud";
		case 0x2b6: return "immortalize";
		case 0x72c: return "bravery";
		case 0x5fe: return "infernal fetters";
		case 0x4da: return "death sentence";
		case 0x4bc: return "disseminate";
		case 0x64a: return "blood sword";
		case 0x80a: return "pom cure";
		case 0x89c: return "ripe banana";
		case 0x986: return "aura curtain";
		case 0x7da: return "lunar dynamo";
		case 0x874: return "Mantle of the Whorl";
		case 0x875: return "Veil of the Whorl";
		case 0x923: return "drain-mob";
		case 0x964: return "head over heals";
		case 0x9ff: return "lunar dynamo 9ff";
		case 0x993: return "frost blade";
		case 0xb91: return "bluefire";
		case 0xcb1: return "Mini - WoD";
		case 0x943: return "Mini";
		case 0xd0e: return "marrow drain";
		case 0xd5b: return "Aetherial Distribution";
		case 0x35b: return "ice spikes";
		case 0x3de: return "bang toss";
		case 0x13cc: return "sanguine bite";
		case 0x156: return "soul drain";
		case 0x1598: return "guzzle";
		case 0x1734: return "retribution";
		case 0x1731: return "seed of the sky";
		case 0xe81: return "scorpion avatar";
		case 0xe80: return "dragonfly avatar";
		case 0xe82: return "beetle avatar";
		case 0xeb1: return "warlord shell";
		case 0x1327: return "brainhurt breakblock";
		case 0x132c: return "meltyspume";
		case 0x13cf: return "brainshaker";
		case 0xfdc: return "explosion";
		case 0x128b: return "bloodstain";
		case 0x3: return "sprint";
		case 0x5: return "teleport";

	}
	char res[256];
	sprintf(res, "(skill id %x)", skillid);
	return res;
}

#define LIGHTER_COLOR(color,how) (((color)&0xFF000000) | (min(255, max(0, (((color)>>16)&0xFF)+(how)))<<16) | (min(255, max(0, (((color)>>8)&0xFF)+(how)))<<8) | (min(255, max(0, (((color)>>0)&0xFF)+(how)))<<0)))

GameDataProcess::GameDataProcess(FFXIVDLL *dll, HANDLE unloadEvent) :
	dll(dll),
	hUnloadEvent(unloadEvent),
	mSent(1048576 * 8),
	mRecv(1048576 * 8),
	mLastIdleTime(0),
	wDPS(*new DPSWindowController()),
	wDOT(*new DOTWindowController()),
	wChat(*new ChatWindowController(dll)) {
	mLastAttack.amount = 0;
	mLastAttack.timestamp = 0;

	mLocalTimestamp = mServerTimestamp = Tools::GetLocalTimestamp();

	hUpdateInfoThreadLock = CreateEvent(NULL, false, false, NULL);
	hUpdateInfoThread = CreateThread(NULL, NULL, GameDataProcess::UpdateInfoThreadExternal, this, NULL, NULL);

	TCHAR ffxivPath[MAX_PATH];
	GetModuleFileNameEx(GetCurrentProcess(), NULL, ffxivPath, MAX_PATH);
	bool kor = wcsstr(ffxivPath, L"KOREA") ? true : false;
	version = kor ? 340 : 400;

	// default colors

	wDPS.statusMap[CONTROL_STATUS_DEFAULT] = { 0, 0, 0x40000000, 0 };
	wDPS.statusMap[CONTROL_STATUS_HOVER] = { 0, 0, 0x70555555, 0 };
	wDPS.statusMap[CONTROL_STATUS_FOCUS] = { 0, 0, 0x70333333, 0 };
	wDPS.statusMap[CONTROL_STATUS_PRESS] = { 0, 0, 0x70000000, 0 };
	wDOT.statusMap[CONTROL_STATUS_DEFAULT] = { 0, 0, 0x40000000, 0 };
	wDOT.statusMap[CONTROL_STATUS_HOVER] = { 0, 0, 0x70555555, 0 };
	wDOT.statusMap[CONTROL_STATUS_FOCUS] = { 0, 0, 0x70333333, 0 };
	wDOT.statusMap[CONTROL_STATUS_PRESS] = { 0, 0, 0x70000000, 0 };

	HRSRC hResource = FindResource(dll->instance(), MAKEINTRESOURCE(IDR_TEXT_COLORDEF), L"TEXT");
	if (hResource) {
		HGLOBAL hLoadedResource = LoadResource(dll->instance(), hResource);
		if (hLoadedResource) {
			LPVOID pLockedResource = LockResource(hLoadedResource);
			if (pLockedResource) {
				DWORD dwResourceSize = SizeofResource(dll->instance(), hResource);
				if (0 != dwResourceSize) {
					struct membuf : std::streambuf {
						membuf(char* base, std::ptrdiff_t n) {
							this->setg(base, base, base + n);
						}
					} sbuf((char*) pLockedResource, dwResourceSize);
					std::istream in(&sbuf);
					std::string job;
					TCHAR buf[64];
					int r, g, b;
					while (in >> job >> r >> g >> b) {
						std::transform(job.begin(), job.end(), job.begin(), ::toupper);
						MultiByteToWideChar(CP_UTF8, 0, job.c_str(), -1, buf, sizeof(buf) / sizeof(TCHAR));
						mClassColors[buf] = D3DCOLOR_XRGB(r, g, b);
					}
				}
			}
		}
	}

	wDPS.addChild(new OverlayRenderer::Control(L"DPS Table Title Placeholder", CONTROL_TEXT_STRING, DT_LEFT));

	OverlayRenderer::Control *mTable = new OverlayRenderer::Control();
	mTable->layoutDirection = LAYOUT_DIRECTION_VERTICAL_TABLE;
	wDPS.addChild(mTable);
	mTable = new OverlayRenderer::Control();
	mTable->layoutDirection = LAYOUT_DIRECTION_VERTICAL_TABLE_SAMESIZE;
	mTable->visible = 0;
	wDPS.addChild(mTable);

	wDPS.layoutDirection = LAYOUT_DIRECTION_VERTICAL;
	wDPS.relativePosition = 1;
	wDOT.layoutDirection = LAYOUT_DIRECTION_VERTICAL_TABLE;
	wDOT.relativePosition = 1;
	wChat.relativePosition = 1;

	wDPS.text = L"DPS";
	wDOT.text = L"DOT";
	wChat.text = L"Chat";

	ReloadLocalization();
}

void GameDataProcess::ReloadLocalization() {
	wDPS.getChild(1)->removeAndDeleteAllChildren();
	wDOT.removeAndDeleteAllChildren();

	OverlayRenderer::Control *mTableHeaderDef = new OverlayRenderer::Control();
	mTableHeaderDef->layoutDirection = LAYOUT_DIRECTION_HORIZONTAL;
	if (wDPS.DpsColumns.jicon) mTableHeaderDef->addChild(new OverlayRenderer::Control(L"", CONTROL_TEXT_RESNAME, DT_CENTER));
	if (wDPS.DpsColumns.uidx) mTableHeaderDef->addChild(new OverlayRenderer::Control(L"#", CONTROL_TEXT_STRING, DT_CENTER));
	if (wDPS.DpsColumns.uname) mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_NAME"), CONTROL_TEXT_STRING, DT_LEFT));
	if (wDPS.DpsColumns.dps) mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_DPS"), CONTROL_TEXT_STRING, DT_CENTER));
	if (wDPS.DpsColumns.total) mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_TOTAL"), CONTROL_TEXT_STRING, DT_CENTER));
	if (wDPS.DpsColumns.crit) mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_CRITICAL"), CONTROL_TEXT_STRING, DT_CENTER));
	if (wDPS.DpsColumns.dh && version == 400) mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_DH"), CONTROL_TEXT_STRING, DT_CENTER));
	if (wDPS.DpsColumns.cdmh) {
		if (version == 340)
			mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_CMH"), CONTROL_TEXT_STRING, DT_CENTER));
		else
			mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_CDMH"), CONTROL_TEXT_STRING, DT_CENTER)); \
	}
	if (wDPS.DpsColumns.max) mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_MAX"), CONTROL_TEXT_STRING, DT_CENTER));
	if (wDPS.DpsColumns.death) mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_DEATH"), CONTROL_TEXT_STRING, DT_CENTER));
	wDPS.getChild(1)->addChild(mTableHeaderDef);
	wDPS.getChild(1)->setPaddingRecursive(wDPS.padding);
	mTableHeaderDef = new OverlayRenderer::Control();
	mTableHeaderDef->layoutDirection = LAYOUT_DIRECTION_HORIZONTAL;
	mTableHeaderDef->addChild(new OverlayRenderer::Control(L"", CONTROL_TEXT_RESNAME, DT_CENTER));
	// mTableHeaderDef->addChild(new OverlayRenderer::Control(L"Source", CONTROL_TEXT_STRING, DT_LEFT));
	mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DOTTABLE_NAME"), CONTROL_TEXT_STRING, DT_LEFT));
	mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DOTTABLE_SKILL"), CONTROL_TEXT_STRING, DT_CENTER));
	mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DOTTABLE_TIME"), CONTROL_TEXT_STRING, DT_RIGHT));
	wDOT.addChild(mTableHeaderDef);
	wDPS.setPaddingRecursive(wDPS.padding);
}

GameDataProcess::~GameDataProcess() {
	WaitForSingleObject(hUpdateInfoThread, -1);
	CloseHandle(hUpdateInfoThread);
	CloseHandle(hUpdateInfoThreadLock);
}

int GameDataProcess::getDoTPotency(int dot) {
	if (version == 340) {
		switch (dot) {
			case 0xf8: return 30;
			case 0xf4: return 20;
			case 0x77: return 30;
			case 0x76: return 35;
			case 0x6a: return 25;
			case 0xf6: return 50;
			case 0x7c: return 40;
			case 0x81: return 50;
			case 0x8f: return 25;
			case 0x90: return 50;
			case 0xa1: return 40;
			case 0xa2: return 40;
			case 0xa3: return 40;
			case 0xb3: return 40;
			case 0xb4: return 35;
			case 0xbd: return 35;
			case 0xbc: return 10;
			case 0x13a: return 20;
			case 0x12: return 50;
			case 0xec: return 20;
			case 0x1ec: return 30;
			case 0x1fc: return 40;
			case 0x356: return 44;
			case 0x2e5: return 40;
			case 0x346: return 40;
			case 0x34b: return 45;
			case 0x2d5: return 50;
			case 0x31e: return 40;
		}
	} else if (version == 400) {
		switch (dot) {
			case 0xf8: return 30;
			case 0xf6: return 50;
			case 0x76: return 35;
			case 0x7c: return 40;
			case 0x81: return 50;
			case 0x8f: return 30;
			case 0x90: return 50;
			case 0xa1: return 40;
			case 0xa2: return 30;
			case 0x2d5: return 60;
			case 0x4b0: return 45;
			case 0x4b1: return 55;
			case 0x31e: return 40;
			case 0xa3: return 40;
			case 0x4ba: return 30;
			case 0xb3: return 40;
			case 0xb4: return 35;
			case 0xbd: return 35;
			case 0x4be: return 40;
			case 0x4bf: return 40;
			case 0x1fc: return 40;
			case 0x346: return 40;
			case 0x34b: return 50;
			case 0xec: return 20;
			case 0x13a: return 20;
		}
	}
	return 0;
}

int GameDataProcess::getDoTDuration(int skill) {
	if (version == 340) {
		switch (skill) {
			case 0xf8: return 15000;
			case 0xf4: return 30000;
			case 0x77: return 24000;
			case 0x76: return 30000;
			case 0x6a: return 30000;
			case 0xf6: return 21000;
			case 0x7c: return 18000;
			case 0x81: return 18000;
			case 0x8f: return 18000;
			case 0x90: return 12000;
			case 0xa1: return 18000;
			case 0xa2: return 21000;
			case 0xa3: return 24000;
			case 0xb3: return 18000;
			case 0xb4: return 24000;
			case 0xbd: return 30000;
			case 0xbc: return 15000;
			case 0x13a: return 15000;
			case 0x12: return 15000;
			case 0xec: return 18000;
			case 0x1ec: return 30000;
			case 0x356: return 30000;
			case 0x2e5: return 30000;
			case 0x346: return 18000;
			case 0x34b: return 30000;
			case 0x2d5: return 24000;
			case 0x31e: return 24000;
		}
	} else if (version == 400) {
		switch (skill) {
			case 0xf8: return 15000;
			case 0xf6: return 18000;
			case 0x76: return 30000;
			case 0x7c: return 18000;
			case 0x81: return 18000;
			case 0x8f: return 18000;
			case 0x90: return 18000;
			case 0xa1: return 18000;
			case 0xa2: return 21000;
			case 0x4b0: return 30000;
			case 0x4b1: return 30000;
			case 0x31e: return 24000;
			case 0xa3: return 24000;
			case 0x4ba: return 18000;
			case 0xb3: return 18000;
			case 0xb4: return 24000;
			case 0xbd: return 30000;
			case 0x4be: return 30000;
			case 0x4bf: return 30000;
			case 0x346: return 18000;
			case 0x34b: return 30000;
			case 0xec: return 18000;
			case 0x13a: return 15000;
		}
	}
	return 0;
}


inline int GameDataProcess::GetActorType(int id) {
	if (mActorPointers.find(id) == mActorPointers.end())
		return -1;
	char *c = (char*) (mActorPointers[id]);
	c += dll->memory()->Struct.Actor.type;
	return *c;
}

inline int GameDataProcess::GetTargetId(int type) {
	__try {
		char *c = ((char*) dll->memory()->Result.Data.TargetMap) + type;
		if (c == 0)
			return NULL_ACTOR;
		char* ptr = (char*)*(PVOID*) c;
		if (ptr == 0)
			return NULL_ACTOR;
		return *(int*) (ptr + dll->memory()->Struct.Actor.id);
	} __except (1) {
		return 0;
	}
}

inline std::string GameDataProcess::GetActorName(int id) {
	if (id == SOURCE_LIMIT_BREAK)
		return "(Limit Break)";
	if (id == NULL_ACTOR)
		return "(null)";
	if (mActorPointers.find(id) == mActorPointers.end()) {
		char t[256];
		sprintf(t, "%08X", id);
		return t;
	}
	char *c = (char*) (mActorPointers[id]);
	c += dll->memory()->Struct.Actor.name;
	if (!Tools::TestValidString(c))
		return "(error)";
	if (strlen(c) == 0)
		return "(empty)";
	return c;
}

inline TCHAR* GameDataProcess::GetActorJobString(int id) {
	if (id >= 35 || id < 0) {

		if (id == SOURCE_LIMIT_BREAK)
			return L"LB";
		if (mActorPointers.find(id) == mActorPointers.end())
			return L"(?)";
		char *c = (char*) (mActorPointers[id]);
		c += dll->memory()->Struct.Actor.job;
		if (!Tools::TestValidString(c))
			return L"(??)";
		id = *c;
	}
	switch (id) {
		case 1: return L"GLD";
		case 2: return L"PGL";
		case 3: return L"MRD";
		case 4: return L"LNC";
		case 5: return L"ARC";
		case 6: return L"CNJ";
		case 7: return L"THM";
		case 8: return L"CPT";
		case 9: return L"BSM";
		case 10: return L"ARM";
		case 11: return L"GSM";
		case 12: return L"LTW";
		case 13: return L"WVR";
		case 14: return L"ALC";
		case 15: return L"CUL";
		case 16: return L"MIN";
		case 17: return L"BTN";
		case 18: return L"FSH";
		case 19: return L"PLD";
		case 20: return L"MNK";
		case 21: return L"WAR";
		case 22: return L"DRG";
		case 23: return L"BRD";
		case 24: return L"WHM";
		case 25: return L"BLM";
		case 26: return L"ACN";
		case 27: return L"SMN";
		case 28: return L"SCH";
		case 29: return L"ROG";
		case 30: return L"NIN";
		case 31: return L"MCH";
		case 32: return L"DRK";
		case 33: return L"AST";
		case 34: return L"SAM";
		case 35: return L"RDM";
	}
	return L"(???)";
}

bool GameDataProcess::IsParseTarget(uint32_t id) {

	if (id == SOURCE_LIMIT_BREAK && dll->config().ParseFilter != 3) return true;
	if (id == 0 || id == NULL_ACTOR || GetActorType(id) != ACTOR_TYPE_PC) return false;
	if (!dll->hooks() || !dll->hooks()->GetOverlayRenderer() || dll->config().ParseFilter == 0) return true;
	if (version == 340) {
		struct PARTY_STRUCT {
			struct {
				union {
					struct {
						uint32_t _u0[4];
						uint32_t id;
						uint32_t _u1[3];
						char* name;
					};
#ifdef _WIN64
					char _u[544];
#else
					char _u[528];
#endif;
				};
			} members[24];
		};
		__try {
			PARTY_STRUCT* ptr = (PARTY_STRUCT*) ((size_t) dll->memory()->Result.Data.PartyMap);
			if (ptr == 0) return true;
			if (id == mSelfId) return true;
			if (dll->config().ParseFilter == 3) return false; // me only
			for (int i = 0; i < 8; i++)
				if (ptr->members[i].id == id)
					return true;
			if (dll->config().ParseFilter == 2) return false; // party
			for (int i = 8; i < 24; i++)
				if (ptr->members[i].id == id)
					return true;
			return false; // alliance
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		}
	} else {
		struct PARTY_STRUCT {
			struct {
				union {
					struct {
						uint64_t _u0[4];
						uint32_t id;
						uint32_t _u1[3];
						char* name;
					};
					char _u[544];
				} members[24];
			};
		};
		__try {
			PARTY_STRUCT* ptr = (PARTY_STRUCT*) ((size_t) dll->memory()->Result.Data.PartyMap);
			if (ptr == 0) return true;
			if (id == mSelfId) return true;
			if (dll->config().ParseFilter == 3) return false; // me only
			for (int i = 0; i < 8; i++)
				if (ptr->members[i].id == id)
					return true;
			if (dll->config().ParseFilter == 2) return false; // party
			for (int i = 8; i < 24; i++)
				if (ptr->members[i].id == id)
					return true;
			return false; // alliance
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		}
	}

	return true; // show anyway if there was any error
}

void GameDataProcess::ResolveUsers() {
	int limit = 1372;
	mSelfId = 0;
	mDamageRedir.clear();
	mActorPointers.clear();
	for (int i = 0; i < limit; i++) {
		__try {
			char* ptr = ((char**) dll->memory()->Result.Data.ActorMap)[i];
#ifdef _WIN64
			if (~(size_t) ptr == 0 || (sizeof(char*) == 8 && (long long) ptr < 0x30000))
				continue;
#else
			if (~(size_t) ptr == 0 || (sizeof(char*) == 4 && (int) ptr < 0x30000))
				continue;
#endif
			if (!Tools::TestValidPtr(ptr, 4))
				continue;
			int id = *(int*) (ptr + dll->memory()->Struct.Actor.id);
			int owner = *(int*) (ptr + dll->memory()->Struct.Actor.owner);
			int type = *(char*) (ptr + dll->memory()->Struct.Actor.type);
			if (mSelfId == 0)
				mSelfId = id;
			if (owner != NULL_ACTOR)
				mDamageRedir[id] = owner;
			else
				mDamageRedir[id] = id;
			mActorPointers[id] = ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		}
	}
}

void GameDataProcess::ResetMeter() {
	mLastAttack.timestamp = 0;
}

void GameDataProcess::CalculateDps(uint64_t timestamp) {
	timestamp = mLastAttack.timestamp;

	if (mLastIdleTime != timestamp) {
		mCalculatedDamages.clear();
		for (auto it = mDpsInfo.begin(); it != mDpsInfo.end(); ++it) {
			mCalculatedDamages.push_back(std::pair<int, int>(it->first, it->second.totalDamage.def + it->second.totalDamage.ind));
		}
		std::sort(mCalculatedDamages.begin(), mCalculatedDamages.end(), [](const std::pair<int, int> &a, const std::pair<int, int> &b) -> bool {
			return a.second > b.second;
		});
	}
}

void GameDataProcess::AddDamageInfo(TEMPDMG dmg, bool direct) {
	if (GetActorType(dmg.source) == ACTOR_TYPE_PC && IsParseTarget(dmg.source)) {

		if (mLastAttack.timestamp < dmg.timestamp - mCombatResetTime) {
			mDpsInfo.clear();
			mActorInfo.clear ();
			mLastIdleTime = dmg.timestamp - 1000;
		}
		mLastAttack = dmg;

		mActorInfo[dmg.source].name = GetActorName (dmg.source);
		mActorInfo[dmg.source].job = GetActorJobString (dmg.source);

		if (direct) {
			mDpsInfo[dmg.source].totalDamage.def += dmg.amount;
			if (mDpsInfo[dmg.source].maxDamage.amount < dmg.amount)
				mDpsInfo[dmg.source].maxDamage = dmg;
			mDpsInfo[dmg.source].totalDamage.crit += dmg.amount;
			if (dmg.isCrit)
				mDpsInfo[dmg.source].critHits++;
			if (dmg.isDirectHit)
				mDpsInfo[dmg.source].directHits++;
			if (dmg.amount == 0)
				mDpsInfo[dmg.source].missHits++;
			mDpsInfo[dmg.source].totalHits++;
		} else {
			mDpsInfo[dmg.source].totalDamage.ind += dmg.amount;
			mDpsInfo[dmg.source].dotHits++;
			dmg.isCrit = 0;
		}
	}
}

void GameDataProcess::UpdateOverlayMessage() {
	uint64_t timestamp = mServerTimestamp - mLocalTimestamp + Tools::GetLocalTimestamp();
	TCHAR res[32768];
	TCHAR tmp[512];
	int pos = 0;
	if (dll->hooks()->GetOverlayRenderer() == nullptr)
		return;

	/* // Debug info
	mDpsInfo.clear();
	mLastIdleTime = timestamp - 1000;
	for (int i = 19; i <= 24+19-1; i++) {
		char test[256];
		int p = sprintf(test, "usr%i", i);
		for (int j = 0; j < i / 2; j++)
			p += sprintf(test + p, "a");
		mDpsInfo[i].totalDamage.ind = 100;
		mDpsInfo[i].totalDamage.def = 100 * i;
		mDpsInfo[i].totalDamage.crit = min(i * i, mDpsInfo[i].totalDamage.def+ mDpsInfo[i].totalDamage.ind);
		mDpsInfo[i].critHits = i * 2;
		mDpsInfo[i].missHits = i;
		mDpsInfo[i].totalHits = i * 3;
		mDpsInfo[i].maxDamage.dmg = i * 1000;
		mDpsInfo[i].maxDamage.isCrit = i % 2;
		mDpsInfo[i].deaths = i;
		mDpsInfo[i].dotHits = (int) (i*1.5);
		mActorInfo[i].job = GetActorJobString(i);
		mActorInfo[i].name = test;
	}
	//*/
	{
		CalculateDps(timestamp);

		std::lock_guard<std::recursive_mutex> guard(dll->hooks()->GetOverlayRenderer()->GetRoot()->getLock());
		{
			SYSTEMTIME s1, s2;
			Tools::MillisToLocalTime(timestamp, &s1);
			Tools::MillisToSystemTime((uint64_t) (timestamp*EORZEA_CONSTANT), &s2);
			if (mShowTimeInfo)
				pos = swprintf(res, sizeof (tmp) / sizeof (tmp[0]), L"FPS %lld / LT %02d:%02d:%02d / ET %02d:%02d:%02d / ",
				(uint64_t) dll->hooks()->GetOverlayRenderer()->GetFPS(),
					(int) s1.wHour, (int) s1.wMinute, (int) s1.wSecond,
					(int) s2.wHour, (int) s2.wMinute, (int) s2.wSecond);
			if (!mDpsInfo.empty()) {
				uint64_t dur = mLastAttack.timestamp - mLastIdleTime;
				int total = 0;
				for (auto it = mCalculatedDamages.begin(); it != mCalculatedDamages.end(); ++it)
					total += it->second;
				pos += swprintf(res + pos, sizeof (tmp) / sizeof (tmp[0]) - pos, L"%02d:%02d.%03d%s / %.2f (%d)",
					(int) ((dur / 60000) % 60), (int) ((dur / 1000) % 60), (int) (dur % 1000),
					mLastAttack.timestamp < timestamp - mCombatResetTime ? L"" : (
						GetTickCount() % 600 < 200 ? L".  " :
						GetTickCount() % 600 < 400 ? L".. " : L"..."
						),
					total * 1000. / (mLastAttack.timestamp - mLastIdleTime), (int) mCalculatedDamages.size());
			} else {
				pos += swprintf(res + pos, sizeof (tmp) / sizeof (tmp[0]) - pos, L"00:00:000 / 0 (0)");
			}
			wDPS.getChild(0, CHILD_TYPE_NORMAL)->text = res;
			OverlayRenderer::Control &wTable = *wDPS.getChild(1, CHILD_TYPE_NORMAL);
			OverlayRenderer::Control &wTable24 = *wDPS.getChild(2, CHILD_TYPE_NORMAL);
			while (wTable.getChildCount() > 1) delete wTable.removeChild(1);
			wTable24.removeAndDeleteAllChildren();

			if (!mDpsInfo.empty() && mLastAttack.timestamp > mLastIdleTime) {
				uint32_t i = 0;
				uint32_t disp = 0;
				bool dispme = false;
				size_t mypos = 0;
				if (mCalculatedDamages.size() > 8) {
					for (auto it = mCalculatedDamages.begin(); it != mCalculatedDamages.end(); ++it, mypos++)
						if (it->first == mSelfId)
							break;
					mypos = max(4, mypos);
					mypos = min(mCalculatedDamages.size() - 4, mypos);
				}
				float maxDps = (float) (mCalculatedDamages.begin()->second * 1000. / (mLastAttack.timestamp - mLastIdleTime));
				int displayed = 0;

				for (auto it = mCalculatedDamages.begin(); it != mCalculatedDamages.end(); ++it) {
					i++;
					if (mCalculatedDamages.size() > wDPS.simpleViewThreshold && it->first != mSelfId)
						continue;
					else if (!dll->hooks()->GetOverlayRenderer()->GetUseDrawOverlayEveryone() && it->first != mSelfId)
						continue;
					OverlayRenderer::Control &wRow = *(new OverlayRenderer::Control());
					TEMPDMG &max = mDpsInfo[it->first].maxDamage;
					float curDps = (float) (it->second * 1000. / (mLastAttack.timestamp - mLastIdleTime));

					wRow.layoutDirection = CONTROL_LAYOUT_DIRECTION::LAYOUT_DIRECTION_HORIZONTAL;
					if (wDPS.DpsColumns.jicon)
						wRow.addChild(new OverlayRenderer::Control(mActorInfo[it->first].job, CONTROL_TEXT_RESNAME, DT_CENTER));
					if (maxDps > 0) {
						wRow.addChild(new OverlayRenderer::Control(curDps / maxDps, 1, LIGHTER_COLOR((mClassColors[mActorInfo[it->first].job] & 0xFFFFFF) | 0x70000000, 0x10), CHILD_TYPE_BACKGROUND);
						wRow.addChild(new OverlayRenderer::Control((mDpsInfo[it->first].totalDamage.def * 1000.f / (mLastAttack.timestamp - mLastIdleTime)) / maxDps, 1, LIGHTER_COLOR((mClassColors[mActorInfo[it->first].job] & 0xFFFFFF) | 0x70000000, 0x20), CHILD_TYPE_BACKGROUND);
						wRow.addChild(new OverlayRenderer::Control((mDpsInfo[it->first].totalDamage.crit * 1000.f / (mLastAttack.timestamp - mLastIdleTime)) / maxDps, 1, LIGHTER_COLOR((mClassColors[mActorInfo[it->first].job] & 0xFFFFFF) | 0x70000000, 0x30), CHILD_TYPE_BACKGROUND);
					}
					if (wDPS.DpsColumns.uidx) {
						swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%d", i);
						wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					}
					if (wDPS.DpsColumns.uname) {
						if (dll->config().hideOtherUserName && it->first != mSelfId)
							wcscpy(tmp, L"...");
						else if (dll->config().SelfNameAsYOU && it->first == mSelfId)
							wcscpy(tmp, L"YOU");
						else
							MultiByteToWideChar(CP_UTF8, 0, mActorInfo[it->first].name.c_str(), -1, tmp, sizeof(tmp) / sizeof(TCHAR));
						auto uname = new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_LEFT);
						uname->maxWidth = wDPS.maxNameWidth;
						wRow.addChild(uname);
					}
					if (wDPS.DpsColumns.dps) {
						swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%.2f", curDps);
						wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					}
					if (wDPS.DpsColumns.total) {
						swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%d", it->second);
						wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					}
					if (wDPS.DpsColumns.crit) {
						swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%.2f%%", mDpsInfo[it->first].totalHits == 0 ? 0.f : (100.f * mDpsInfo[it->first].critHits / mDpsInfo[it->first].totalHits));
						wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					}
					if (wDPS.DpsColumns.dh && version == 400) {
						swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%.2f%%", mDpsInfo[it->first].totalHits == 0 ? 0.f : (100.f * mDpsInfo[it->first].directHits / mDpsInfo[it->first].totalHits));
						wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					}
					if (wDPS.DpsColumns.cdmh) {
						if (version == 340)
							swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%d/%d/%d", mDpsInfo[it->first].critHits, mDpsInfo[it->first].missHits, mDpsInfo[it->first].totalHits + mDpsInfo[it->first].dotHits);
						else
							swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%d/%d/%d/%d", mDpsInfo[it->first].critHits, mDpsInfo[it->first].directHits, mDpsInfo[it->first].missHits, mDpsInfo[it->first].totalHits + mDpsInfo[it->first].dotHits);
						wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					}
					if (wDPS.DpsColumns.max) {
						if (version == 400 && (max.isCrit || max.isDirectHit))
							swprintf(tmp, sizeof(tmp) / sizeof(tmp[0]), L"%d%s%s", max.amount, max.isDirectHit ? L"." : L"", max.isCrit ? L"!" : L"");
						else
							swprintf(tmp, sizeof(tmp) / sizeof(tmp[0]), L"%d%s", max.amount, max.isCrit ? L"!" : L"");
						wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					}
					if (wDPS.DpsColumns.death) {
						swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%d", mDpsInfo[it->first].deaths);
						wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					}
					wTable.addChild(&wRow);
				}
				wTable.setPaddingRecursive(wDPS.padding);
				if (wTable24.visible = (mCalculatedDamages.size() > wDPS.simpleViewThreshold)) {

					OverlayRenderer::Control *wRow24 = new OverlayRenderer::Control();
					wRow24->layoutDirection = CONTROL_LAYOUT_DIRECTION::LAYOUT_DIRECTION_HORIZONTAL;
					i = 0;
					for (auto it = mCalculatedDamages.begin(); it != mCalculatedDamages.end(); ++it) {
						i++;
						float curDps = (float) (it->second * 1000. / (mLastAttack.timestamp - mLastIdleTime));
						OverlayRenderer::Control &wCol24 = *(new OverlayRenderer::Control());
						wCol24.layoutDirection = CONTROL_LAYOUT_DIRECTION::LAYOUT_DIRECTION_HORIZONTAL;
						wCol24.addChild(new OverlayRenderer::Control(mActorInfo[it->first].job, CONTROL_TEXT_RESNAME, DT_CENTER));
						if (dll->config().hideOtherUserName && it->first != mSelfId)
							wcscpy(tmp, L"...");
						else if (dll->config().SelfNameAsYOU && it->first == mSelfId)
							wcscpy(tmp, L"YOU");
						else
							MultiByteToWideChar(CP_UTF8, 0, mActorInfo[it->first].name.c_str(), -1, tmp, sizeof(tmp) / sizeof(TCHAR));
						OverlayRenderer::Control *uname = new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_LEFT);
						uname->maxWidth = wDPS.maxNameWidth;
						wCol24.addChild(uname);
						swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%.2f", curDps);
						wCol24.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_LEFT));
						wCol24.addChild(new OverlayRenderer::Control(1.f, 1.f, 0x90000000), CHILD_TYPE_BACKGROUND);
						wCol24.addChild(new OverlayRenderer::Control(curDps / maxDps, 1, LIGHTER_COLOR((mClassColors[mActorInfo[it->first].job] & 0xFFFFFF) | 0x70000000, 0x10), CHILD_TYPE_BACKGROUND);
						wCol24.addChild(new OverlayRenderer::Control((mDpsInfo[it->first].totalDamage.def * 1000.f / (mLastAttack.timestamp - mLastIdleTime)) / maxDps, 1, LIGHTER_COLOR((mClassColors[mActorInfo[it->first].job] & 0xFFFFFF) | 0x70000000, 0x30), CHILD_TYPE_BACKGROUND);
						wCol24.addChild(new OverlayRenderer::Control((mDpsInfo[it->first].totalDamage.crit * 1000.f / (mLastAttack.timestamp - mLastIdleTime)) / maxDps, 1, LIGHTER_COLOR((mClassColors[mActorInfo[it->first].job] & 0xFFFFFF) | 0x70000000, 0x40), CHILD_TYPE_BACKGROUND);


						wCol24.setPaddingRecursive(wDPS.padding / 2);
						wCol24.padding = wDPS.padding / 2;
						wCol24.margin = wDPS.padding;
						wRow24->addChild(&wCol24);
						if (wRow24->getChildCount() == 3) {
							wTable24.addChild(wRow24);
							wRow24 = new OverlayRenderer::Control();
							wRow24->layoutDirection = CONTROL_LAYOUT_DIRECTION::LAYOUT_DIRECTION_HORIZONTAL;
						}
					}
					wRow24->padding = 0;
					wTable24.padding = 0;
					if (wRow24->getChildCount() == 0)
						delete wRow24;
					else
						wTable24.addChild(wRow24);
				}
			}
		}
		{
			std::vector<TEMPBUFF> buff_sort;
			for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ) {
				if (it->expires < timestamp)
					it = mActiveDoT.erase(it);
				else {
					if (it->source == mSelfId && it->target != mSelfId && it->potency > 0) {
						buff_sort.push_back(*it);
					}
					++it;
				}
			}
			while (wDOT.getChildCount() > 1)
				delete wDOT.removeChild(1);
			if (!buff_sort.empty()) {
				int currentTarget = GetTargetId(dll->memory()->Struct.Target.current);
				int focusTarget = GetTargetId(dll->memory()->Struct.Target.focus);
				int hoverTarget = GetTargetId(dll->memory()->Struct.Target.hover);
				std::sort(buff_sort.begin(), buff_sort.end(), [&](const TEMPBUFF & a, const TEMPBUFF & b) {
					if ((a.target == focusTarget) ^ (b.target == focusTarget))
						return (a.target == focusTarget ? 1 : 0) > (b.target == focusTarget ? 1 : 0);
					if ((a.target == currentTarget) ^ (b.target == currentTarget))
						return (a.target == currentTarget ? 1 : 0) > (b.target == currentTarget ? 1 : 0);
					if ((a.target == hoverTarget) ^ (b.target == hoverTarget))
						return (a.target == hoverTarget ? 1 : 0) > (b.target == hoverTarget ? 1 : 0);
					return a.expires < b.expires;
				});
				size_t i = 0;
				for (auto it = buff_sort.begin(); it != buff_sort.end(); ++it, i++) {
					if (i > 9 && it->target != currentTarget && it->target != hoverTarget && it->target != focusTarget)
						break;
					OverlayRenderer::Control &wRow = *(new OverlayRenderer::Control());
					OverlayRenderer::Control *col;
					wRow.layoutDirection = CONTROL_LAYOUT_DIRECTION::LAYOUT_DIRECTION_HORIZONTAL;
					wRow.addChild(col = new OverlayRenderer::Control(currentTarget == it->target ? L"T" :
						hoverTarget == it->target ? L"M" :
						focusTarget == it->target ? L"F" : L"", CONTROL_TEXT_STRING, DT_CENTER));

					// MultiByteToWideChar(CP_UTF8, 0, GetActorName(it->source).c_str(), -1, tmp, sizeof(tmp) / sizeof(TCHAR));
					// wRow.addChild(col = new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));

					MultiByteToWideChar(CP_UTF8, 0, GetActorName(it->target).c_str(), -1, tmp, sizeof(tmp) / sizeof(TCHAR));
					wRow.addChild(col = new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					wRow.addChild(col = new OverlayRenderer::Control(Languages::getDoTName(it->buffid), CONTROL_TEXT_STRING, DT_CENTER));
					swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%.1fs%s",
						(it->expires - timestamp) / 1000.f,
						it->simulated ? (
						(getDoTDuration(it->buffid) + (it->contagioned ? 15000 : 0)) < it->expires - timestamp
							? L"(x)" :
							L"(?)") : L"");
					wRow.addChild(col = new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					if (it->expires - timestamp < 8000) {
						int v = 0xB0 - (0xB0 * (int) (it->expires - timestamp) / 8000);
						v = 255 - max(0, min(255, v));
						wRow.getChild(wRow.getChildCount() - 1)->statusMap[0].foreground =
							D3DCOLOR_ARGB(0xff, 0xff, v, v);
					}
					wDOT.addChild(&wRow);
				}
				i = buff_sort.size() - i;
				if (i > 0) {
					OverlayRenderer::Control &wRow = *(new OverlayRenderer::Control());
					wRow.layoutDirection = CONTROL_LAYOUT_DIRECTION::LAYOUT_DIRECTION_HORIZONTAL;
					wRow.addChild(new OverlayRenderer::Control(L"", CONTROL_TEXT_STRING, DT_CENTER));
					wRow.addChild(new OverlayRenderer::Control(L"", CONTROL_TEXT_STRING, DT_CENTER));
					swprintf(tmp, sizeof(tmp) / sizeof(tmp[0]), L"%zd", i);
					wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					wRow.addChild(new OverlayRenderer::Control(L"", CONTROL_TEXT_STRING, DT_CENTER));
					wDOT.addChild(&wRow);
				}
			}
		}
		if (!mWindowsAdded) {
			mWindowsAdded = true;
			dll->hooks()->GetOverlayRenderer()->AddWindow(&wDPS);
			dll->hooks()->GetOverlayRenderer()->AddWindow(&wDOT);
			dll->hooks()->GetOverlayRenderer()->AddWindow(&wChat);
		}
		wDOT.setPaddingRecursive(wDOT.padding);
	}
}

void GameDataProcess::ProcessAttackInfo(int source, int target, int skill, ATTACK_INFO *info, uint64_t timestamp) {
	switch (skill) {
		case 0xc8:
		case 0xc9:
		case 0xcb:
		case 0xcc:
		case 0x1eb5:
		case 0x1eb6:
		case 0xc7:
		case 0x1090:
		case 0x1091:
		case 0xca:
		case 0x1092:
		case 0x1093:
		case 0x108f:
		case 0x108e:
		case 0x1095:
		case 0x1094:
		case 0xcd:
		case 0x1096:
		case 0xd0:
		case 0x1097:
		case 0x1098:
			source = SOURCE_LIMIT_BREAK;
			break;
	}
	for (int i = 0; i < 8; i++) {
		if (info->attack[i].swingtype == 0) continue;

		TEMPDMG dmg = { 0 };
		dmg.isDoT = false;
		dmg.timestamp = timestamp;
		dmg.skill = skill;
		if (mDamageRedir.find(source) != mDamageRedir.end())
			source = mDamageRedir[source];
		dmg.source = source;
		switch (info->attack[i].swingtype) {
			case 1:
			case 10:
				if (info->attack[i].damage == 0) {
					dmg.amount = 0;
					AddDamageInfo(dmg, true);
				}
				break;
			case 5:
				dmg.isCrit = true;
			case 3:
				/*
				{
					char t[512];
					sprintf(t, "/e attack %016llx", *(uint64_t*) &info->attack[i]);
					dll->addChat(t);
				}
				//*/
				dmg.amount = info->attack[i].damage;
				if (info->attack[i].mult10)
					dmg.amount *= 10;
				AddDamageInfo(dmg, true);
				break;
			case 17:
			case 18:
			{
				int buffId = info->attack[i].damage;
				dmg.buffId = buffId;
				dmg.isDoT = true;

				if (getDoTPotency(buffId) == 0)
					continue; // not an attack buff

				bool add = true;
				for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ) {
					if (it->source == source && it->target == target && it->buffid == buffId) {
						it->applied = timestamp;
						it->expires = timestamp + getDoTDuration(buffId) + mDotApplyDelayEstimation[it->buffid].get();
						it->simulated = 1;
						it->contagioned = 0;
						add = false;
						break;
					} else if (it->expires < timestamp) {
						it = mActiveDoT.erase(it);
					} else
						++it;
				}
				if (add) {
					TEMPBUFF b;
					b.buffid = buffId;
					b.source = source;
					b.target = target;
					b.applied = timestamp;
					b.potency = info->attack[i].buffAmount;
					b.critRate = info->attack[i].critAmount / 1000.f;
					b.potency = (int) (b.potency * (1 + b.critRate));
					b.expires = timestamp + getDoTDuration(buffId) + mDotApplyDelayEstimation[buffId].get();
					b.potency = getDoTPotency(b.buffid);
					b.simulated = 1;
					b.contagioned = 0;
					mActiveDoT.push_back(b);
				}

				break;
			}
			case 41:
			{ // Probably contagion
				if (skill == 795) { // it is contagion
					auto it = mActiveDoT.begin();
					while (it != mActiveDoT.end()) {
						if (it->expires - mContagionApplyDelayEstimation.get() < timestamp) {
							it = mActiveDoT.erase(it);
						} else if (it->source == source && it->target == target) {
							it->applied = timestamp;
							it->expires += 15000;
							it->simulated = 1;
							it->contagioned = 1;
							++it;
						} else
							++it;
					}
				}
				break;
			}
		}
	}
}

void GameDataProcess::ProcessAttackInfo(int source, int target, int skill, ATTACK_INFO_V4 *info, uint64_t timestamp) {
	switch (skill) {
		case 0xc8:
		case 0xc9:
		case 0xcb:
		case 0xcc:
		case 0x1eb5:
		case 0x1eb6:
		case 0xc7:
		case 0x1090:
		case 0x1091:
		case 0xca:
		case 0x1092:
		case 0x1093:
		case 0x108f:
		case 0x108e:
		case 0x1095:
		case 0x1094:
		case 0xcd:
		case 0x1096:
		case 0xd0:
		case 0x1097:
		case 0x1098:
			source = SOURCE_LIMIT_BREAK;
			break;
	}
	for (int i = 0; i < 8; i++) {
		if (info->attack[i].swingtype == 0) continue;

		TEMPDMG dmg = { 0 };
		dmg.isDoT = false;
		dmg.timestamp = timestamp;
		dmg.skill = skill;
		if (mDamageRedir.find(source) != mDamageRedir.end())
			source = mDamageRedir[source];
		dmg.source = source;
		switch (info->attack[i].swingtype) {

			case 1: case 3: case 5: case 6:
				dmg.amount = info->attack[i].damage;
				if (info->attack[i].mult10)
					dmg.amount *= 10;
				dmg.isCrit = info->attack[i].isCrit;
				dmg.isDirectHit = info->attack[i].isDirectHit;
				if (dmg.amount == 0 && info->attack[i].swingtype == 1)
					dmg.isMiss = 1;
				if (dmg.amount > 10000) {
					char test[512];
					sprintf(test, "/e [%s] %d/%s: %d / %16llX", GetActorName(dmg.source).c_str(), info->attack[i].swingtype, debug_skillname(dmg.skill).c_str(), dmg.amount, *reinterpret_cast<uint64_t*>(&(info->attack[i])));
					dll->addChat(test);
				}
				AddDamageInfo(dmg, true);
				/*
				if (dmg.source == mSelfId) {
					char test[512];
					sprintf(test, "/e %s: %d / %2X", debug_skillname(dmg.skill).c_str(), dmg.dmg, (int) info->attack[i]._u5);
					dll->addChat(test);
				}
				//*/
				break;
			case 15:
			case 16:
			case 51:
			{
				int buffId = info->attack[i].damage;

				if (getDoTPotency(buffId) == 0)
					continue; // not an attack buff

				/*{
					char test[512];
					sprintf(test, "/e %s: %d %d %d %d", debug_skillname(skill).c_str()
						, (int) info->attack[i].buffAmount
						, (int) info->attack[i].critAmount
						, (int) info->attack[i]._u1
						, (int) info->attack[i]._u4);
					dll->addChat(test);
				}//*/

				bool add = true;
				for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ) {
					if (it->source == source && it->target == target && it->buffid == buffId) {
						it->applied = timestamp;
						it->expires = timestamp + getDoTDuration(buffId) + mDotApplyDelayEstimation[it->buffid].get();
						it->simulated = 1;
						it->contagioned = 0;
						add = false;
						break;
					} else if (it->expires < timestamp) {
						it = mActiveDoT.erase(it);
					} else
						++it;
				}
				if (add) {
					TEMPBUFF b;
					b.buffid = buffId;
					b.source = source;
					b.target = target;
					b.applied = timestamp;
					b.expires = timestamp + getDoTDuration(buffId) + mDotApplyDelayEstimation[buffId].get();
					b.potency = info->attack[i].buffAmount;
					b.critRate = info->attack[i].critAmount / 1000.f;
					b.potency = (int) (b.potency * (1 + b.critRate));
					b.simulated = 1;
					b.contagioned = 0;
					mActiveDoT.push_back(b);
				}

				break;
			}
		}
	}
}

void GameDataProcess::ProcessGameMessage(void *data, uint64_t timestamp, size_t len, bool setTimestamp) {

	if (setTimestamp) {
		mServerTimestamp = timestamp;
		mLocalTimestamp = Tools::GetLocalTimestamp();
	} else
		timestamp = mServerTimestamp;

	GAME_MESSAGE *msg = (GAME_MESSAGE*) data;
	switch (msg->message_cat1) {
		case GAME_MESSAGE::C1_Combat:
		{
			switch (msg->message_cat2) {
				case GAME_MESSAGE::C2_ActorInfo:
					break;
				case GAME_MESSAGE::C2_Info1:
					if (msg->Combat.Info1.c1 == 23 && msg->Combat.Info1.c2 == 3) {
						if (msg->Combat.Info1.c5 == 0) {
							int total = 0;
							for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it) {
								total += it->potency;
							}
							if (total > 0) {
								for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it) {
									int mine = msg->Combat.Info1.c3 * it->potency / total;
									if (mine > 0) {
										TEMPDMG dmg;
										dmg.timestamp = timestamp;
										dmg.source = it->source;
										dmg.buffId = it->buffid;
										dmg.isDoT = true;
										dmg.amount = mine;
										if (mDamageRedir.find(dmg.source) != mDamageRedir.end())
											dmg.source = mDamageRedir[dmg.source];
										AddDamageInfo(dmg, false);
									}
								}
							}
						} else {
							// sprintf(tss, "/e c5=%d", msg->Combat.Info1.c5); dll->addChat(tss);
							for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it) {
								// sprintf(tss, "/e %d/%d, %d/%d", it->source, it->target, it->buffid, msg->Combat.Info1.c5); dll->addChat(tss);
								if (it->source == it->target && it->buffid == msg->Combat.Info1.c5) {
									int mine = msg->Combat.Info1.c3;
									if (mine > 0) {
										TEMPDMG dmg;
										dmg.timestamp = timestamp;
										dmg.source = it->source;
										dmg.buffId = it->buffid;
										dmg.isDoT = true;
										dmg.amount = mine;
										if (mDamageRedir.find(dmg.source) != mDamageRedir.end())
											dmg.source = mDamageRedir[dmg.source];
										AddDamageInfo(dmg, false);
									}
								}
							}
						}
					} else if (msg->Combat.Info1.c1 == 21) {
						auto it = mActiveDoT.begin();
						while (it != mActiveDoT.end()) {
							if (it->expires < timestamp)
								it = mActiveDoT.erase(it);
							else
								++it;
						}
					} else if (msg->Combat.Info1.c1 == 6) {
						// death
						int who = msg->Combat.Info1.c5;
						if (IsParseTarget(msg->actor))
							mDpsInfo[msg->actor].deaths++;
						/*
						sprintf(tss, "cmsgDeath %s killed %s", getActorName(who), getActorName(msg->actor));
						dll->pipe()->sendInfo(tss);
						//*/
						auto it = mActiveDoT.begin();
						while (it != mActiveDoT.end()) {
							if (it->expires < timestamp || it->target == msg->actor)
								it = mActiveDoT.erase(it);
							else
								++it;
						}
					}
					break;
				case GAME_MESSAGE::C2_AddBuff:
					for (int i = 0; i < msg->Combat.AddBuff.buff_count && i < 5; i++) {
						// Track non-attack buffs too for ground AoEs
						// if (getDoTPotency(msg->Combat.AddBuff.buffs[i].buffID) == 0) continue; // not an attack buff
						auto it = mActiveDoT.begin();
						bool add = true;
						while (it != mActiveDoT.end()) {
							if (it->source == msg->Combat.AddBuff.buffs[i].actorID && it->target == msg->actor && it->buffid == msg->Combat.AddBuff.buffs[i].buffID) {
								uint64_t nexpire = timestamp + (int) (msg->Combat.AddBuff.buffs[i].duration * 1000);
								if (it->simulated) {
									if (it->contagioned) {
										mContagionApplyDelayEstimation.add((int) (timestamp - it->applied));
										//sprintf(tss, "cmsg => simulated (Contagion: %d / %d)", nexpire - it->expires, estimatedContagionDelay.get());
									} else {
										mDotApplyDelayEstimation[it->buffid].add((int) (timestamp - it->applied));
										//sprintf(tss, "cmsg => simulated (%d: %d / %d)", it->buffid, nexpire - it->expires, estimatedDelays[it->buffid].get());
									}
									//dll->pipe()->sendInfo(tss);
								}
								it->applied = timestamp;
								it->expires = nexpire;
								it->simulated = 0;
								it->contagioned = 0;
								add = false;
								break;
							} else if (it->expires < timestamp) {
								it = mActiveDoT.erase(it);
							} else
								++it;
						}
						if (add) {
							TEMPBUFF b;
							b.buffid = msg->Combat.AddBuff.buffs[i].buffID;
							b.source = msg->Combat.AddBuff.buffs[i].actorID;
							b.target = msg->actor;
							b.applied = timestamp;
							b.expires = timestamp + (int) (msg->Combat.AddBuff.buffs[i].duration * 1000);
							b.potency = getDoTPotency(b.buffid);
							b.contagioned = 0;
							b.simulated = 0;
							mActiveDoT.push_back(b);
							/*
							char tss[512];
							sprintf(tss, "/e AddBuff %s->%s / %d (%.2f) %04X %04X %04x", GetActorName(msg->Combat.AddBuff.buffs[i].actorID).c_str(), GetActorName(msg->actor).c_str(), (int) b.buffid, msg->Combat.AddBuff.buffs[i].duration, (int) msg->Combat.AddBuff.buffs[i]._u1, (int) msg->Combat.AddBuff.buffs[i]._u2, (int) msg->Combat.AddBuff.buffs[i].extra);
							dll->addChat(tss);
							//*/
						}
					}
					break;
				case GAME_MESSAGE::C2_UseAbility:
					if (version == 340)
						ProcessAttackInfo(msg->actor, msg->Combat.UseAbility.target, msg->Combat.UseAbility.skill, &msg->Combat.UseAbility.attack, timestamp);
					break;
				case GAME_MESSAGE::C2_UseAoEAbility:
					if (version == 340) {
						if (GetActorName(msg->actor), msg->Combat.UseAoEAbility.skill == 174) { // Bane
							SimulateBane(timestamp, msg->actor, 16, msg->Combat.UseAoEAbility.targets, msg->Combat.UseAoEAbility.attack);
						} else {
							for (int i = 0; i < 16; i++)
								if (msg->Combat.UseAoEAbility.targets[i].target != 0) {
									ProcessAttackInfo(msg->actor, msg->Combat.UseAoEAbility.targets[i].target, msg->Combat.UseAoEAbility.skill, &msg->Combat.UseAoEAbility.attack[i], timestamp);
								}
						}
					}
					break;
				case GAME_MESSAGE::C2_UseAbilityV4T1:
				case GAME_MESSAGE::C2_UseAbilityV4T8:
				case GAME_MESSAGE::C2_UseAbilityV4T16:
				case GAME_MESSAGE::C2_UseAbilityV4T24:
				case GAME_MESSAGE::C2_UseAbilityV4T32:
					if (version == 400) {
						int count = msg->Combat.UseAoEAbilityV4.attackCount;
						TARGET_STRUCT *targets;
						switch (msg->message_cat2) {
							case GAME_MESSAGE::C2_UseAbilityV4T1:
								count = max(count, 1);
								targets = (TARGET_STRUCT*) ((char*) msg + 34 * 4);
								break;
							case GAME_MESSAGE::C2_UseAbilityV4T8:
								count = max(count, 8);
								targets = (TARGET_STRUCT*) ((char*) msg + 146 * 4);
								break;
							case GAME_MESSAGE::C2_UseAbilityV4T16:
								count = max(count, 16);
								targets = (TARGET_STRUCT*) ((char*) msg + 274 * 4);
								break;
							case GAME_MESSAGE::C2_UseAbilityV4T24:
								count = max(count, 24);
								targets = (TARGET_STRUCT*) ((char*) msg + 402 * 4);
								break;
							case GAME_MESSAGE::C2_UseAbilityV4T32:
								count = max(count, 32);
								targets = (TARGET_STRUCT*) ((char*) msg + 530 * 4);
								break;
						}
						for (int i = 0; i < msg->Combat.UseAoEAbilityV4.attackCount; i++) {
							if (targets[i].target != 0 && targets[i].target != NULL_ACTOR) {
								ProcessAttackInfo(msg->actor, targets[i].target, msg->Combat.UseAoEAbilityV4.skill, &msg->Combat.UseAoEAbilityV4.attack[i], timestamp);
							}
						}
						/*
						if (GetActorName(msg->actor), msg->Combat.UseAoEAbilityV4.skill == 174) { // Bane
							SimulateBane(timestamp, msg->actor, 16, msg->Combat.UseAoEAbilityV4.targets, msg->Combat.UseAoEAbilityV4.attack);
						} else {
							for (int i = 0; i < 16; i++)
								if (msg->Combat.UseAoEAbilityV4.targets[i].target != 0) {
									ProcessAttackInfo(msg->actor, msg->Combat.UseAoEAbilityV4.targets[i].target, msg->Combat.UseAoEAbilityV4.skill, &msg->Combat.UseAoEAbilityV4.attack[i], timestamp);
								}
						}
						//*/
					}
					break;
				default:
					goto DEFPRT;
			}
			break;
		}
DEFPRT:
		default:
			/*
			int pos = sprintf(tss, "/e unknown %04X:%04X (%s, %x) ", (int)msg->message_cat1, (int)msg->message_cat2, GetActorName(msg->actor), msg->length);
			if (msg->actor == 0x102464F7 || strcmp(GetActorName(msg->actor).c_str(), "Striking Dummy") == 0) {
				int pos = sprintf(tss, "/e U %04X:%04X (%s) ", (int) msg->message_cat1, (int) msg->message_cat2, GetActorName(msg->actor).c_str());
				for (int i = 0; i < msg->length && i < 64; i++)
					pos += sprintf(tss + pos, "%02X ", (int) ((unsigned char*) msg)[i]);
				dll->addChat(tss);
			}
			//*/
			break;
	}
}

void GameDataProcess::SimulateBane(uint64_t timestamp, uint32_t actor, int maxCount, TARGET_STRUCT* targets, ATTACK_INFO* attacks) {
	int baseActor = NULL_ACTOR;
	for (int i = 0; i < 16; i++)
		if (targets[i].target != 0)
			for (int j = 0; j < 4; j++)
				if (attacks[i].attack[j].swingtype == 11)
					baseActor = targets[i].target;
	if (baseActor != NULL_ACTOR) {
		for (int i = 0; i < 16; i++)
			if (targets[i].target != 0)
				for (int j = 0; j < 4; j++)
					if (attacks[i].attack[j].swingtype == 17) {
						TEMPBUFF buff;
						uint64_t expires = -1;
						for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it)
							if (it->buffid == attacks[i].attack[j].damage &&
								it->source == actor &&
								it->target == baseActor) {
								expires = it->expires;
								break;
							}
						if (expires != -1) {
							bool addNew = true;
							for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it)
								if (it->buffid == attacks[i].attack[j].damage &&
									it->source == actor &&
									it->target == targets[i].target) {
									it->applied = timestamp;
									it->expires = expires;
									it->simulated = 1;
									addNew = false;
									break;
								}
							if (addNew) {
								buff.buffid = attacks[i].attack[j].damage;
								buff.applied = timestamp;
								buff.expires = expires;
								buff.source = actor;
								buff.target = targets[i].target;
								buff.potency = getDoTPotency(buff.buffid);
								buff.simulated = 1;

								mActiveDoT.push_back(buff);
							}
						}
					}
	}

}

void GameDataProcess::SimulateBane(uint64_t timestamp, uint32_t actor, int maxCount, TARGET_STRUCT* targets, ATTACK_INFO_V4* attacks) {
	int baseActor = NULL_ACTOR;
	for (int i = 0; i < 16; i++)
		if (targets[i].target != 0)
			for (int j = 0; j < 4; j++)
				if (attacks[i].attack[j].swingtype == 11)
					baseActor = targets[i].target;
	if (baseActor != NULL_ACTOR) {
		for (int i = 0; i < 16; i++)
			if (targets[i].target != 0)
				for (int j = 0; j < 4; j++)
					if (attacks[i].attack[j].swingtype == 17) {
						TEMPBUFF buff;
						uint64_t expires = -1;
						for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it)
							if (it->buffid == attacks[i].attack[j].damage &&
								it->source == actor &&
								it->target == baseActor) {
								expires = it->expires;
								break;
							}
						if (expires != -1) {
							bool addNew = true;
							for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it)
								if (it->buffid == attacks[i].attack[j].damage &&
									it->source == actor &&
									it->target == targets[i].target) {
									it->applied = timestamp;
									it->expires = expires;
									it->simulated = 1;
									addNew = false;
									break;
								}
							if (addNew) {
								buff.buffid = attacks[i].attack[j].damage;
								buff.applied = timestamp;
								buff.expires = expires;
								buff.source = actor;
								buff.target = targets[i].target;
								buff.potency = getDoTPotency(buff.buffid);
								buff.simulated = 1;

								mActiveDoT.push_back(buff);
							}
						}
					}
	}

}

void GameDataProcess::PacketErrorMessage(int signature, int length) {
	/*
	char t[1024];
	sprintf(t, "cmsgError: packet error %08X / %08X", signature, length);
	//*/
}
int tryInflate(z_stream *inf, int flags) {
	__try {
		return inflate(inf, Z_NO_FLUSH);
	} __except (EXCEPTION_EXECUTE_HANDLER) {}
	return -1;
}

void GameDataProcess::ParsePacket(Tools::ByteQueue &p, bool setTimestamp) {

	struct {
		union {
			uint32_t signature;		// 0 ~ 3		0x41A05252
			uint64_t signature_2[2];
		};
		uint64_t timestamp;		// 16 ~ 23
		uint32_t length;		// 24 ~ 27
		uint16_t _u2;			// 28 ~ 29
		uint16_t message_count;	// 30 ~ 31
		uint8_t _u3;			// 32
		uint8_t is_gzip;		// 33
		uint16_t _u4;			// 34 ~ 35
		uint32_t _u5;			// 36 ~ 39;
		uint8_t data[65536];
	} packet;

	while (p.getUsed() >= 28) {
		p.peek(&packet, 28);
		if ((packet.signature != 0x41A05252 && (packet.signature_2[0] | packet.signature_2[1])) || packet.length > sizeof(packet) || packet.length <= 40) {
			p.waste(1);
			PacketErrorMessage(packet.signature, packet.length);
			continue;
		}
		if (p.getUsed() < packet.length)
			break;
		p.peek(&packet, packet.length);
		unsigned char *buf = nullptr;
		if (packet.is_gzip) {
			z_stream inflater;
			memset(&inflater, 0, sizeof(inflater));
			inflater.avail_out = DEFLATE_CHUNK_SIZE;
			inflater.next_out = inflateBuffer;
			inflater.next_in = packet.data;
			inflater.avail_in = packet.length - 40;

			if (inflateInit(&inflater) == Z_OK) {
				int res = tryInflate(&inflater, Z_NO_FLUSH);
				if (Z_STREAM_END != res) {
					inflateEnd(&inflater);
					p.waste(1);
					continue;
				}
				inflateEnd(&inflater);
				if (inflater.avail_out == 0) {
					p.waste(1);
					continue;
				}
				buf = inflateBuffer;
			}
		} else {
			buf = packet.data;
		}
		if (packet.signature == 0x41A05252) {
			if (buf != nullptr) {
				for (int i = 0; i < packet.message_count; i++) {
					ProcessGameMessage(buf, packet.timestamp, ((char*) &packet.data - (char*) &packet), setTimestamp);
					dll->sendPipe(setTimestamp ? "recv" : "send", (char*) buf, *((uint32_t*) buf));
					buf += *((uint32_t*) buf);
				}
			}
		}
		p.waste(packet.length);
	}
}

void GameDataProcess::UpdateInfoThread() {
	while (WaitForSingleObject(hUnloadEvent, 0) == WAIT_TIMEOUT) {
		WaitForSingleObject(hUpdateInfoThreadLock, 50);
		__try {
			ResolveUsers();
			ParsePacket(mSent, false);
			ParsePacket(mRecv, true);
			UpdateOverlayMessage();
		} __except (1) {
			dll->addChat("/e UpdateInfoThread Error");
		}
	}
	TerminateThread(GetCurrentThread(), 0);
}