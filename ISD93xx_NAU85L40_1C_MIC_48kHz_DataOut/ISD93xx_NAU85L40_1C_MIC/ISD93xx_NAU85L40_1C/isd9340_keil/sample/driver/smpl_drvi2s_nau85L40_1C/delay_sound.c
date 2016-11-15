#include "delay_sound.h"
#include "math.h"
#include "FixedBeamformer.h"
#include <stdlib.h>
#include "Beamforming.h"

#define BUFFER_SIZE 64000
#define ORDER 3

delay * delay_init(double num_samples_to_delay, double fractional_delay, double fb_v, double dtm_v, double delay_blend_param, int mic) {
	delay *d = malloc (sizeof(delay));
	if(d == NULL){return NULL;}
	else{
		d->buffer = malloc(sizeof (double) * BUFFER_SIZE);
		if(d->buffer == NULL){return NULL;}
		else{
			for(int i = 0; i < BUFFER_SIZE ; i++){
				d->buffer[i] = 0;
			}
		}
	}
	delay_set_delay(d, num_samples_to_delay, fractional_delay);
	delay_set_feedback_volume(d, fb_v);
	delay_set_delay_tap_mix_volume(d, dtm_v);
	delay_set_blend_param(d, delay_blend_param);
	delay_set_mic(d, mic);
	d->write_pointer = &d->buffer[BUFFER_SIZE - 1];
	return d;
}

void delay_set_delay(delay *d, double num_samples_to_delay, double fractional_delay){
	if(fractional_delay > 0.5){
		num_samples_to_delay +=1;
		fractional_delay-=1;
	}
	fractional_delay += ORDER;
	float tc = fractional_delay;
	
	d->thiran_coeff_1 = -3 * (tc -3)/(tc+1);
	d->thiran_coeff_2 = 3 * (tc-2) * (tc - 3)/((tc + 1) * (tc+2));
	d->thiran_coeff_3 = -1 * (tc -1) * (tc -2) * (tc - 3)/((tc + 1) * (tc + 2) * (tc + 3));
	
	for(int i = 0; i < ORDER; i++){
		d->prev_in[i] = 0;
		d->prev_out[i] = 0;
	}	

	d->num_samples_to_delay = num_samples_to_delay;
	d->fractional_delay = fractional_delay;
}

void delay_set_feedback_volume(delay *d,double fb_v){
	d->feedback_volume = fb_v;
}

void delay_set_delay_tap_mix_volume(delay *d,double dtm_v){
	d->delay_tap_mix_volume = dtm_v;
}

void delay_set_mic(delay *d,int mic){
	d->mic = mic;	
}

void delay_set_blend_param(delay *d, double delay_blend_param){
	d->delay_blend_param = delay_blend_param;
}

void delete_delay(delay *d){
	if(d != NULL){
		free(d->buffer);
		free(d);
	}
}

double delay_out(delay *d, double in) {
	double out;
	double x;

	*d->write_pointer = in;
		
	/*apply integer part of delay*/
	d->read_pointer = d->write_pointer - (int)d->num_samples_to_delay;

	/*loop buffer*/
	if(d->read_pointer < d->buffer){
		d->read_pointer = d->read_pointer + BUFFER_SIZE - 1;
	}

	x = *(d->read_pointer);
	out =	  (d->prev_in[0]                   )
		+ (d->prev_in[1]  * d->thiran_coeff_1)
		+ (d->prev_in[2]  * d->thiran_coeff_2)
		+ (x              * d->thiran_coeff_3)
		- (d->prev_out[2] * d->thiran_coeff_1)
		- (d->prev_out[1] * d->thiran_coeff_2)
		- (d->prev_out[0] * d->thiran_coeff_3);
	
	d->prev_in[0]  = d->prev_in[1];
	d->prev_in[1]  = d->prev_in[2];
	d->prev_in[2]  = x;
	d->prev_out[0] = d->prev_out[1];
	d->prev_out[1] = d->prev_out[2];
	d->prev_out[2] = out;

		
	d->write_pointer = d->write_pointer + 1;

	if((d->write_pointer - &d->buffer[0]) > BUFFER_SIZE - 1) {
		d->write_pointer = &d->buffer[0];
	}
	return out;
}
