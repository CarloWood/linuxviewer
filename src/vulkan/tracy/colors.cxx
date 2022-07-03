#include "sys.h"
#include "colors.h"
#include "threadsafe/aithreadsafe.h"
#include <map>
#include <functional>

namespace tracy {

int colors[max_color] = {
  // Hand picked colors.
  0x9966CC, // #9966CC : Amethyst 
  0xFFBF00, // #FFBF00 : Amber 
  0xE52B50, // #E52B50 : Amaranth 
  0x7FFFD4, // #7FFFD4 : Aquamarine 
  0x89CFF0, // #89CFF0 : Baby blue 
  0xCD7F32, // #CD7F32 : Bronze 
  0x007BA7, // #007BA7 : Cerulean 
  0x7FFF00, // #7FFF00 : Chartreuse green 
  0x6F4E37, // #6F4E37 : Coffee 
  0xFF7F50, // #FF7F50 : Coral 
  0x50C878, // #50C878 : Emerald 
  0xFFD700, // #FFD700 : Gold 
  0xB57EDC, // #B57EDC : Lavender 
  0xBFFF00, // #BFFF00 : Lime 
  0xFF00FF, // #FF00FF : Magenta 
  0xFF6600, // #FF6600 : Orange 

  // Add more colors, just in case.
  0xF0F8FF, // #F0F8FF : Alice blue
  0xFBCEB1, // #FBCEB1 : Apricot 
  0x007FFF, // #007FFF : Azure 
  0xF5F5DC, // #F5F5DC : Beige 
  0xCB4154, // #CB4154 : Brick red 
//  0x000000, // #000000 : Black 
  0x0000FF, // #0000FF : Blue 
  0x0095B6, // #0095B6 : Blue-green 
  0x8A2BE2, // #8A2BE2 : Blue-violet 
  0xDE5D83, // #DE5D83 : Blush 
  0x993300, // #993300 : Brown 
  0x800020, // #800020 : Burgundy 
  0x702963, // #702963 : Byzantium 
  0x960018, // #960018 : Carmine 
  0xDE3163, // #DE3163 : Cerise 
  0xF7E7CE, // #F7E7CE : Champagne 
  0x7B3F00, // #7B3F00 : Chocolate 
  0x0047AB, // #0047AB : Cobalt blue 
  0xB87333, // #B87333 : Copper 
  0xDC143C, // #DC143C : Crimson 
  0x00FFFF, // #00FFFF : Cyan 
  0xEDC9A, // #EDC9A : Desert sand 
  0x7DF9FF, // #7DF9FF : Electric blue 
  0x00FF3F, // #00FF3F : Erin 
  0xBEBEBE, // #BEBEBE : Gray 
  0x008001, // #008001 : Green 
  0x3FFF00, // #3FFF00 : Harlequin 
  0x4B0082, // #4B0082 : Indigo 
  0xFFFFF0, // #FFFFF0 : Ivory 
  0x00A86B, // #00A86B : Jade 
  0x29AB87, // #29AB87 : Jungle green 
  0xFFF700, // #FFF700 : Lemon 
  0xC8A2C8, // #C8A2C8 : Lilac 
  0xFF00AF, // #FF00AF : Magenta rose 
  0x800000, // #800000 : Maroon 
  0xE0B0FF, // #E0B0FF : Mauve 
  0x000080, // #000080 : Navy blue 
  0xCC7722, // #CC7722 : Ochre 
  0x808000, // #808000 : Olive 
  0xFF4500, // #FF4500 : Orange-red 
  0xDA70D6, // #DA70D6 : Orchid 
  0xFFE5B4, // #FFE5B4 : Peach 
  0xD1E231, // #D1E231 : Pear 
  0xCCCCFF, // #CCCCFF : Periwinkle 
  0x1C39BB, // #1C39BB : Persian blue 
  0xFFC0CB, // #FFC0CB : Pink 
  0x8E4585, // #8E4585 : Plum 
  0x003153, // #003153 : Prussian blue 
  0xCC8899, // #CC8899 : Puce 
  0x6A0DAD, // #6A0DAD : Purple 
  0xE30B5C, // #E30B5C : Raspberry 
  0xFF0000, // #FF0000 : Red 
  0xC71585, // #C71585 : Red-violet 
  0xFF007F, // #FF007F : Rose 
  0xE0115F, // #E0115F : Ruby 
  0xFA8072, // #FA8072 : Salmon 
  0x92000A, // #92000A : Sangria 
  0x0F52BA, // #0F52BA : Sapphire 
  0xFF2400, // #FF2400 : Scarlet 
  0xC0C0C0, // #C0C0C0 : Silver 
  0x708090, // #708090 : Slate gray 
  0xA7FC00, // #A7FC00 : Spring bud 
  0x00FF7F, // #00FF7F : Spring green 
  0xD2B48C, // #D2B48C : Tan 
  0x483C32, // #483C32 : Taupe 
  0x008080, // #008080 : Teal 
  0x40E0D0, // #40E0D0 : Turquoise 
  0x3F00FF, // #3F00FF : Ultramarine 
  0x7F00FF, // #7F00FF : Violet 
  0x40826D, // #40826D : Viridian 
  0xFFFFFF, // #FFFFFF : White 
  0xFFFF00, // #FFFF00 : Yellow 
};

static std::atomic_int next_color_index = 0;

int get_next_color()
{
  return color(next_color_index++);
}

using container_type = std::map<uint64_t, int>;
using name_hash_to_color_map_type = aithreadsafe::Wrapper<container_type, aithreadsafe::policy::Primitive<std::mutex>>;
static name_hash_to_color_map_type name_hash_to_color_map;

int get_color(uint64_t hash)
{
  name_hash_to_color_map_type::wat w_name_hash_to_color_map(name_hash_to_color_map);
  auto entry = w_name_hash_to_color_map->find(hash);
  if (entry != w_name_hash_to_color_map->end())
    return entry->second;
  int color = get_next_color();
  w_name_hash_to_color_map->insert(container_type::value_type(hash, color));
  return color;
}

} // namespace tracy
