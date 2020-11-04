/*
 * PI controller

 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
