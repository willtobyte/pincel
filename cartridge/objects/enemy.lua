return {
  animations = {
    walk = { { 0, 200 }, { 1, 200 } },
  },

  atlas = "world",

  on_spawn = function(self)
    self.health = 100
  end,

  on_collision = function(self, name, kind)
    print("[COLLISION] " .. self.name .. " (" .. self.kind .. ") hit " .. name .. " (" .. kind .. ")")
  end,

  on_collision_end = function(self, name, kind)
    print("[COLLISION END] " .. self.name .. " (" .. self.kind .. ") left " .. name .. " (" .. kind .. ")")
  end,

  on_damage = function(self, amount)
    self.health = self.health - amount
    print("damage! health is now " .. self.health)
  end,
}
