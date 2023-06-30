#pragma once
const char *MASTER_PARAMDEF = R"###(
{
	"Global": [
	{
		"name": "timeout",
		"description": "max number of seconds for full execution. 0=unlimited"
	},
	{
		"name": "solver",
		"description": "solver engine to be used. Requires support at compilation time",
		"type": "str",
		"values": ["cplex", "gurobi", "coin"],
		"default": "cplex"
	}
	],
	"OPP": [
	{
		"name": "RunUpit",
		"description": "Enable the computation of u-pit before solving",
		"type": "bool",
		"default": true
	},
	{
		"name": "RTWMovement",
		"description": "Number of periods to advance per RTW iteration",
		"type": "int",
		"default": 50000
	},
	{
		"name":"AggregationPattern",
		"description": "Pattern for aggregation",
		"type": "list_int",
		"default": [1,1,1,2,2,4,8]
	}
	]
}
)###";
