# Copyright (C) 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

project(
  'percetto',
  ['c', 'cpp'],
  version: '0.1.0',
  license: 'Apache-2.0',
  meson_version: '>=0.54.0',
  default_options: ['buildtype=debug',
                    'warning_level=2',
                    'werror=true',
                    'c_std=c11',
                    'cpp_std=c++2a'],
)

threads_dep = dependency('threads')

if (meson.is_subproject())
  # for some reason the default is not picked up when
  # this is a subproject
  add_project_arguments('-std=c++2a', language : 'cpp')
endif

subdir('perfetto-sdk')
subdir('src')
if (not meson.is_subproject())
  # no need for the examples when this is a sub project
  subdir('examples')
endif
