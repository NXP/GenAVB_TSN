/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2021 NXP
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *    Neither the name of NXP Semiconductors nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * DOC: PI controller
 *
 * Implements a PI controller of the form:
 * u(t) = Kp * err(t) + Ki * sum(err(t'))
 *
 * err(t) - error value (the difference between process variable (measured) and setpoint(expected))
 * u(t) - control variable
 * Kp - proportional gain
 * Ki - integral gain
 *
 * with several restrictions:
 * err(t) and u(t) are 32bit integers
 * Kp and Ki are of the form 1/(2^kp), 1/(2^ki)
 */

/**
 * pi_reset() - Resets the PI controller algorithm
 * @p - pointer to pi structure
 * @u - estimated control variable value
 *
 * The function resets the PI so that the control variable (u(t)) becomes equal
 * to a user provided value. This assumes the user has knowledge of the system
 * being modeled and can make a good estimate of the control variable value.
 */
void pi_reset(struct pi *p, int u)
{
	p->err = 0;
	p->integral = ((int64_t)u) << p->ki;

	p->u = p->integral >> p->ki;
}

/**
 * pi_init() - Initializes the PI controller algorithm
 * @p - pointer to pi structure
 * @ki - integral term (the true Ki is 1/(2^ki))
 * @kp - proportional term (the true Kp is 1/(2^kp))
 *
 * Called once to initialize the Kp/Ki terms of the PI
 */
void pi_init(struct pi *p, unsigned int ki, unsigned int kp)
{
	if (ki > 31)
		ki = 31;

	if (kp > 31)
		kp = 31;

	p->ki = ki;
	p->kp = kp;
}

/**
 * pi_update() - Updates the PI controller algorithm
 * @p - pointer to pi structure
 * @err - error sample
 *
 * Updates the PI with a new err(t) sample
 * Returns the new value of the control variable u(t)
 */
int pi_update(struct pi *p, int err)
{
	p->err = err;
	p->integral += err;

	p->u = (p->err >> p->kp) + (p->integral >> p->ki);

	return p->u;
}
