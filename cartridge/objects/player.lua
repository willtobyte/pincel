return {
  animations = {
    walk = { atlas = "world", { 0, 200 }, { 1, 200 } },
  },

  on_spawn = function(self)
    self.health = 100
  end,

  on_loop = function(self, delta)
    self.x = self.x + gamepad.left.x
    self.y = self.y + gamepad.left.y

    if gamepad.south then self:damage(10) end
  end,

  on_animation_end = function(self, animation)
    --
  end,

  on_collision = function(self, name, kind)
    gamepad:rumble(0.5, 0.5, 200)
  end,

  on_collision_end = function(self, name, kind)
  end,

  on_screen_exit = function(self, direction)
  end,

  on_screen_enter = function(self, direction)
  end,

  on_damage = function(self, amount)
    self.health = self.health - amount
    print("damage! health is now " .. self.health)
  end,
}
