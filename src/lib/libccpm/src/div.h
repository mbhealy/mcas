/*
   Copyright [2019] [IBM Corporation]
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef CCPM_DIV_H
#define CCPM_DIV_H

template <typename T, typename U>
	T div_round_up(T t, U u)
	{
		return (t + (u - U(1)))/u;
	}

template <typename T, typename U>
	T round_up(T t, U u)
	{
		return div_round_up(t, u) * u;
	}

#endif
