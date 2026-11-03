using System.Collections.Generic;
using Godot;

namespace HiveWeGodot;

public static class TileThumbnails
{
    private static readonly Dictionary<string, Texture2D?> cache = new();

    public static Texture2D? Get(string id)
    {
        if (cache.TryGetValue(id, out Texture2D? cached))
        {
            return cached;
        }
        Image? image = HiveCore.TerrainTileImage(id);
        Texture2D? texture = image == null ? null : ImageTexture.CreateFromImage(image);
        cache[id] = texture;
        return texture;
    }
}
