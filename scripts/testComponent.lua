local testComponent = {}

local tickn = 0

function testComponent.tick()
	tickn = tickn + 1
	--print('tick', tickn)
end

return testComponent
