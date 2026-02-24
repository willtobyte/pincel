return {
  animations = {
    walk = { { 0, 200 }, { 1, 200 } },
  },

  atlas = "char",

  on_spawn = function(self)
    self.health = 100
  end,

  on_loop = function(self, delta)
    if keyboard.w then self.y = self.y - 1 end
    if keyboard.s then self.y = self.y + 1 end
    if keyboard.a then self.x = self.x - 1 end
    if keyboard.d then self.x = self.x + 1 end

    if keyboard.space then self:damage(10) end
  end,

  on_animation_end = function(self, animation)
    print("animation ended: " .. animation)
  end,

  on_damage = function(self, amount)
    self.health = self.health - amount
    print("damage! health is now " .. self.health)
  end,
}
