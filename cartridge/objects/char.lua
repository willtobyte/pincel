return {
  atlas = "char",

  walk = { { 0, 200 }, { 1, 200 } },

  on_damage = function(self, amount)
    self.health = self.health - amount
    print("damage! health is now " .. self.health)
  end,
}
