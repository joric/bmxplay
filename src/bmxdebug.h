//BmxPlay project (c) 2001-2012 Joric^Proxium

//bmxplay debug

#include <conio.h>

int BmxDebug()
{
	unsigned int i, j, k, t, r;
	char *gp, *tp, *sp;
	byte b;

	BmxMachine *m;
	BmxPattern *p;
	BmxSequence *s;
	int n;

	//title

	printf("Machines:%d Song length:%d\n\n", bmx.mach.machines, bmx.seq.songsize);

	for (i = 0; i < bmx.mach.machines; i++)
	{
		m = &bmx.mach.m[i];
		printf("Machine %d:%s Channels:%d gpsize:%d tpsize:%d\n", i, m->dllname, m->channels, m->gpsize, m->tpsize);
	}

	//connections
	printf("\nConnections(%d):\n", bmx.con->connections);
	for (i = 0; i < bmx.con->connections; i++)
		printf("%s - %s\n", bmx.mach.m[bmx.con->c[i].src].name, bmx.mach.m[bmx.con->c[i].dst].name);


	//events
	printf("\nEvent list:\n");
	for (j = 0; j < bmx.seq.sequences; j++)
	{
		printf("%-8s", bmx.mach.m[bmx.seq.s[j].machine].name);

		for (k = 0; k < bmx.seq.s[j].events; k++)
		{
			printf("%02X:%02X ", bmx.seq.s[j].data[k].pos, bmx.seq.s[j].data[k].event);
		}
		printf("\n");
	}

	//initial data
	printf("\nInitial data:\n");
	for (j = 0; j < bmx.mach.machines; j++)
	{
		m = &bmx.mach.m[j];
		printf("%-8s", m->name);

		gp = m->gpar;
		for (t = 0; t < m->gpsize; t++)
		{
			b = *gp++;
			printf("%02X", b);
		}
		printf(" ");

		for (t = 0; t < m->tracks; t++)
		{
			tp = m->tpar;
			for (k = 0; k < m->tpsize; k++)
			{
				b = *tp++;
				printf("%02X", b);
			}
			printf(" ");
		}


		if (m->attrs)
			printf(" /Attrs: ");
		for (k = 0; k < m->attrs; k++)
			printf("%s:%d", m->attr[k].name, m->attr[k].value);
		printf("\n");
	}

	printf("\n");

	//patterns
	for (j = 0; j < bmx.mach.machines; j++)
	{
		m = &bmx.mach.m[j];


		for (i = 0; i < bmx.mach.m[j].patterns; i++)
		{
			n = m->firstpattern + i;
			p = &bmx.p[n];

			printf("Pattern: %s (%s)\n", p->name, m->name);
			byte b;

			gp = p->gdata;

			for (int r = 0; r < p->rows; r++)
			{
				for (t = 0; t < m->sources; t++)
				{
					//sp=p->sdata[t] + r*m->channels*2;
					sp = p->sdata[t] + r * 2 * 2;
					//for (k=0; k<m->channels*2; k++) {b=*sp++; printf ("%02X",b);}
					for (k = 0; k < 2 * 2; k++)
					{
						b = *sp++;
						printf("%02X", b);
					}
					printf(" ");
				}

				for (k = 0; k < m->gpsize; k++)
				{
					b = *gp++;
					printf("%02X", b);
				}
				printf(" ");

				for (t = 0; t < m->tracks; t++)
				{
					tp = p->tdata[t] + r * m->tpsize;
					for (k = 0; k < m->tpsize; k++)
					{
						b = *tp++;
						printf("%02X", b);
					}
					printf(" ");
				}

				printf("\n");
			}
		}

	}

	int samples = 0;

	int tick, ticks, pat;
	ticks = bmx.seq.songsize;
	//sequence

	int SamplesPerTick = BmxGetSamplesPerTick();

	for (tick = 0; tick < ticks; tick++)
	{
		printf("Tick %02d:\n", tick);

		for (j = 0; j < bmx.seq.sequences; j++)
		{

			s = &bmx.seq.s[j];
			for (k = 0; k < s->events; k++)
				if (s->data[k].pos == tick)
					s->event = s->data[k].event;

			if (s->events != 0)
			{
				pat = s->event - 0x10;
				m = &bmx.mach.m[s->machine];
				p = &bmx.p[pat + m->firstpattern];

				printf("  %8s:", m->name);
				printf(" Pat:%02X Row:%02d ", pat, s->row);

				//pattern data
				r = s->row;


				for (t = 0; t < m->sources; t++)
				{
					// jeskola saves panning even for mono machine...
					sp = p->sdata[t] + r * 2 * 2;
					for (k = 0; k < 2 * 2; k++)
					{
						b = *sp++;
						printf("%02X", b);
					}
					printf(" ");
				}

				gp = (p->gdata + r * m->gpsize);
				for (t = 0; t < m->gpsize; t++)
				{
					b = *gp++;
					printf("%02X", b);
				}
				printf(" ");

				for (t = 0; t < m->tracks; t++)
				{
					tp = p->tdata[t] + r * m->tpsize;
					for (k = 0; k < m->tpsize; k++)
					{
						b = *tp++;
						printf("%02X", b);
					}
					printf(" ");
				}

				printf("\n");

				samples += SamplesPerTick;

				if (s->event >= 0x10)
				{
					s->row++;
					if (s->row >= p->rows)
						s->row = 0;
				}
			}
		}
	}

	printf("\nTotal: %d samples (%.2f seconds)\n", samples, samples / 44100.0);

////////////////////////Mixing Scheme/////////////////////////////


	for (i = 0; i < bmx.mach.machines; i++)
	{
		m = &bmx.mach.m[i];
		printf("M%d\t%s(%s)\tChannels:%d\tgpsize:%d\ttpsize:%d\n", i, m->name, m->dllname, m->channels, m->gpsize,
		       m->tpsize);
	}

	//connections
	printf("\nConnections(%d):\n", bmx.con->connections);
	for (i = 0; i < bmx.con->connections; i++)
		printf("M%d (%d) - M%d (%d)\n", bmx.con->c[i].src, bmx.mach.m[bmx.con->c[i].src].sources, bmx.con->c[i].dst,
		       bmx.mach.m[bmx.con->c[i].dst].sources);

//printf mixing

	bool busy[256];
	int buffers = 256;
	for (i = 0; i < buffers; i++)
		busy[i] = false;

	int machines = bmx.mach.machines;

	for (i = 0; i < machines; i++)
		bmx.mach.m[i].scount = bmx.mach.m[i].sources;

	printf("\nMixing scheme:\n\n");

	int machine = 0;

	machine = 0;

	//until all sources of Master won't be rendered
	while (bmx.mach.m[0].scount != 0)
	{
		if (bmx.mach.m[machine].scount != 0 || bmx.mach.m[machine].scount < 0)
			machine++;	//next if cannot evaluate yet, or machine has been processed
		else
		{
			printf("M%d -> ", machine);
			for (k = 0; k < bmx.con->connections; k++)
			{
				if (bmx.con->c[k].src == machine)
				{
					printf("M%d (amp:%04X pan:%04X) ", bmx.con->c[k].dst, bmx.con->c[k].amp,
					       bmx.con->c[k].pan);

					bmx.mach.m[bmx.con->c[k].dst].scount--;
				}
			}

			bmx.mach.m[machine].scount--;

			printf("\n");

			machine = 0;
		}
	}
	return 0;
}
