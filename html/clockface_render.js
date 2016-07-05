
var canvas;
var ctx;
var width, height;

function anim_render()
{
	var d = new Date();
	var h = d.getHours();
	var m = d.getMinutes();
	var s = d.getSeconds();

	var dots = new Array();

	var radius = width > height ? height/2 : width/2;
	radius -= 5;

	clockface_draw(clk_ops, h, m, s, 16, dots);

	ctx.clearRect(0, 0, width, height);
	ctx.strokeStyle="#505050";

	ctx.beginPath();
	ctx.arc(width/2, height/2, radius, 0, 2*Math.PI);
	ctx.stroke();

	for (i = 0; i < 60; i++)
	{
		var angle = (i*6-90)/57.3;
		var x = width/2 + Math.cos(angle) * (radius-30);
		var y = height/2 + Math.sin(angle) * (radius-30);
		ctx.fillStyle = "rgb(" + dots[i].r + "," + dots[i].g + "," + dots[i].b + ")";
		ctx.beginPath();
		ctx.arc(x, y, 10, 0, 2*Math.PI);
		ctx.fill();
	}
}

function anim_update(timestamp)
{
	var diff = timestamp - prev_timestamp;

	window.requestAnimationFrame(anim_update);

	if (diff >= 1000)
	{
		anim_render();
		prev_timestamp = timestamp;
	}
}

function anim_resize()
{
	c = document.getElementById("canvas");
	width = window.innerWidth;
  	height = window.innerHeight;

  	canvas.width = width;
  	canvas.height = height;
}

function anim_init()
{
	document.body.style.backgroundColor = 0;

	canvas = document.getElementById("canvas");
  	ctx = canvas.getContext('2d');

  	anim_resize();

	state = 0;
	state_wait_ms = 0;
	prev_timestamp = performance.now();

	window.addEventListener('resize', anim_resize)
	window.requestAnimationFrame(anim_update);
}
