local entities = {}

for i = 1, 10000 do
  entities[i] = {
    kind = "char",
    name = "p" .. i,
    x = (i * 37) % 320,
    y = (i * 13) % 180,
    animation = "walk",
  }
end

entities.on_loop = function(delta)
end

return entities
