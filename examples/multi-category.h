/*
 * Copyright (C) 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MULTI_CATEGORY_H
#define MULTI_CATEGORY_H

#include "percetto.h"

#define MY_PERCETTO_CATEGORIES(C, G) \
  C(cat, "Cat events") \
  C(dog, "Dog events", "slow") \
  C(dbg, "Debug events", "debug", "mytag") \
  G(dbg_or_dog, dbg, dog)

PERCETTO_CATEGORY_DECLARE(MY_PERCETTO_CATEGORIES);

PERCETTO_TRACK_DECLARE(squirrels);

#endif /* MULTI_CATEGORY_H */
