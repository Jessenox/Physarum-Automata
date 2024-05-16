#pragma once

class DensityData {
	public:
		int gen = 0;
		int st0 = 0;
		int st1 = 0;
		int st2 = 0;
		int st3 = 0;
		int st4 = 0;
		int st5 = 0;
		int st6 = 0;
		int st7 = 0;
		int st8 = 0;

		DensityData(int generation, int state0, int state1, int state2, int state3, int state4, int state5, int state6, int state7, int state8) {
			gen = generation;
			st0 = state0;
			st1 = state1;
			st2 = state2;
			st3 = state3;
			st4 = state4;
			st5 = state5;
			st6 = state6;
			st7 = state7;
			st8 = state8;
		};
	private:
	
};
