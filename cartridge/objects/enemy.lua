return {
  animations = {
    walk = { atlas = "world", { 0, 200 }, { 1, 200 } },
  },

  on_spawn = function(self)
    self.health = 100
  end,

  on_collision = function(self, name, kind)
  end,

  on_collision_end = function(self, name, kind)
  end,

  on_damage = function(self, amount)
    self.health = self.health - amount
  end,
}
