/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2016 - Daniel De Matteis
 *  Copyright (C)      2016 - Gregor Richards
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <boolean.h>

#include "netplay_private.h"

#include "../../autosave.h"
#include "../../driver.h"
#include "../../input/input_driver.h"

#if 0
#define DEBUG_NONDETERMINISTIC_CORES
#endif

/**
 * netplay_update_unread_ptr
 *
 * Update the global unread_ptr and unread_frame_count to correspond to the
 * earliest unread frame count of any connected player
 */
void netplay_update_unread_ptr(netplay_t *netplay)
{
   if (netplay->is_server && !netplay->connected_players)
   {
      /* Nothing at all to read! */
      netplay->unread_ptr = netplay->self_ptr;
      netplay->unread_frame_count = netplay->self_frame_count;

   }
   else
   {
      size_t new_unread_ptr = 0;
      uint32_t new_unread_frame_count = (uint32_t) -1;
      uint32_t player;

      for (player = 0; player < MAX_USERS; player++)
      {
         if (!(netplay->connected_players & (1<<player))) continue;
         if (netplay->read_frame_count[player] < new_unread_frame_count)
         {
            new_unread_ptr = netplay->read_ptr[player];
            new_unread_frame_count = netplay->read_frame_count[player];
         }
      }

      if (!netplay->is_server && netplay->server_frame_count < new_unread_frame_count)
      {
         new_unread_ptr = netplay->server_ptr;
         new_unread_frame_count = netplay->server_frame_count;
      }

      netplay->unread_ptr = new_unread_ptr;
      netplay->unread_frame_count = new_unread_frame_count;
   }
}

/**
 * netplay_simulate_input
 * @netplay             : pointer to netplay object
 * @sim_ptr             : frame index for which to simulate input
 * @resim               : are we resimulating, or simulating this frame for the
 *                        first time?
 *
 * "Simulate" input by assuming it hasn't changed since the last read input.
 */
void netplay_simulate_input(netplay_t *netplay, size_t sim_ptr, bool resim)
{
   uint32_t player;
   size_t prev;
   struct delta_frame *simframe, *pframe;

   simframe = &netplay->buffer[sim_ptr];

   for (player = 0; player < MAX_USERS; player++)
   {
      if (!(netplay->connected_players & (1<<player))) continue;
      if (simframe->have_real[player]) continue;

      prev = PREV_PTR(netplay->read_ptr[player]);
      pframe = &netplay->buffer[prev];

      if (resim)
      {
         /* In resimulation mode, we only copy the buttons. The reason for this
          * is nonobvious:
          *
          * If we resimulated nothing, then the /duration/ with which any input
          * was pressed would be approximately correct, since the original
          * simulation came in as the input came in, but the /number of times/
          * the input was pressed would be wrong, as there would be an
          * advancing wavefront of real data overtaking the simulated data
          * (which is really just real data offset by some frames).
          *
          * That's acceptable for arrows in most situations, since the amount
          * you move is tied to the duration, but unacceptable for buttons,
          * which will seem to jerkily be pressed numerous times with those
          * wavefronts.
          */
         const uint32_t keep = (1U<<RETRO_DEVICE_ID_JOYPAD_UP) |
                               (1U<<RETRO_DEVICE_ID_JOYPAD_DOWN) |
                               (1U<<RETRO_DEVICE_ID_JOYPAD_LEFT) |
                               (1U<<RETRO_DEVICE_ID_JOYPAD_RIGHT);
         uint32_t sim_state = simframe->simulated_input_state[player][0] & keep;
         sim_state |= pframe->real_input_state[player][0] & ~keep;
         simframe->simulated_input_state[player][0] = sim_state;
      }
      else
      {
         memcpy(simframe->simulated_input_state[player],
                pframe->real_input_state[player],
                WORDS_PER_INPUT * sizeof(uint32_t));
      }
   }
}

static void netplay_handle_frame_hash(netplay_t *netplay, struct delta_frame *delta)
{
   if (netplay->is_server)
   {
      if (netplay->check_frames &&
          delta->frame % abs(netplay->check_frames) == 0)
      {
         delta->crc = netplay_delta_frame_crc(netplay, delta);
         netplay_cmd_crc(netplay, delta);
      }
   }
   else if (delta->crc && netplay->crcs_valid)
   {
      /* We have a remote CRC, so check it */
      uint32_t local_crc = netplay_delta_frame_crc(netplay, delta);
      if (local_crc != delta->crc)
      {
         if (!netplay->crc_validity_checked)
         {
            /* If the very first check frame is wrong, they probably just don't
             * work */
            netplay->crcs_valid = false;
         }
         else if (netplay->crcs_valid)
         {
            /* Fix this! */
            if (netplay->check_frames < 0)
            {
               /* Just report */
               RARCH_ERR("Netplay CRCs mismatch!\n");
            }
            else
            {
               netplay_cmd_request_savestate(netplay);
            }
         }
      }
      else if (!netplay->crc_validity_checked)
      {
         netplay->crc_validity_checked = true;
      }
   }
}

/**
 * netplay_sync_pre_frame
 * @netplay              : pointer to netplay object
 *
 * Pre-frame for Netplay synchronization.
 */
bool netplay_sync_pre_frame(netplay_t *netplay)
{
   retro_ctx_serialize_info_t serial_info;

   if (netplay_delta_frame_ready(netplay, &netplay->buffer[netplay->self_ptr], netplay->self_frame_count))
   {
      serial_info.data_const = NULL;
      serial_info.data = netplay->buffer[netplay->self_ptr].state;
      serial_info.size = netplay->state_size;

      memset(serial_info.data, 0, serial_info.size);
      if ((netplay->quirks & NETPLAY_QUIRK_INITIALIZATION) || netplay->self_frame_count == 0)
      {
         /* Don't serialize until it's safe */
      }
      else if (!(netplay->quirks & NETPLAY_QUIRK_NO_SAVESTATES) && core_serialize(&serial_info))
      {
         if (netplay->force_send_savestate && !netplay->stall && !netplay->remote_paused)
         {
            /* Send this along to the other side */
            serial_info.data_const = netplay->buffer[netplay->self_ptr].state;
            netplay_load_savestate(netplay, &serial_info, false);
            netplay->force_send_savestate = false;
         }
      }
      else
      {
         /* If the core can't serialize properly, we must stall for the
          * remote input on EVERY frame, because we can't recover */
         netplay->quirks |= NETPLAY_QUIRK_NO_SAVESTATES;
         netplay->stateless_mode = true;
      }

      /* If we can't transmit savestates, we must stall until the client is ready */
      if (netplay->self_frame_count > 0 &&
          (netplay->quirks & (NETPLAY_QUIRK_NO_SAVESTATES|NETPLAY_QUIRK_NO_TRANSMISSION)) &&
          (netplay->connections_size == 0 || !netplay->connections[0].active ||
           netplay->connections[0].mode < NETPLAY_CONNECTION_CONNECTED))
         netplay->stall = NETPLAY_STALL_NO_CONNECTION;
   }

   if (netplay->is_server)
   {
      fd_set fds;
      struct timeval tmp_tv = {0};
      int new_fd;
      struct sockaddr_storage their_addr;
      socklen_t addr_size;
      struct netplay_connection *connection;
      size_t connection_num;

      /* Check for a connection */
      FD_ZERO(&fds);
      FD_SET(netplay->listen_fd, &fds);
      if (socket_select(netplay->listen_fd + 1, &fds, NULL, NULL, &tmp_tv) > 0 &&
          FD_ISSET(netplay->listen_fd, &fds))
      {
         addr_size = sizeof(their_addr);
         new_fd = accept(netplay->listen_fd, (struct sockaddr*)&their_addr, &addr_size);
         if (new_fd < 0)
         {
            RARCH_ERR("%s\n", msg_hash_to_str(MSG_NETPLAY_FAILED));
            goto process;
         }

         /* Set the socket nonblocking */
         if (!socket_nonblock(new_fd))
         {
            /* Catastrophe! */
            socket_close(new_fd);
            goto process;
         }

#if defined(IPPROTO_TCP) && defined(TCP_NODELAY)
         {
            int flag = 1;
            if (setsockopt(new_fd, IPPROTO_TCP, TCP_NODELAY,
#ifdef _WIN32
               (const char*)
#else
               (const void*)
#endif
               &flag,
               sizeof(int)) < 0)
               RARCH_WARN("Could not set netplay TCP socket to nodelay. Expect jitter.\n");
         }
#endif

#if defined(F_SETFD) && defined(FD_CLOEXEC)
         /* Don't let any inherited processes keep open our port */
         if (fcntl(new_fd, F_SETFD, FD_CLOEXEC) < 0)
            RARCH_WARN("Cannot set Netplay port to close-on-exec. It may fail to reopen if the client disconnects.\n");
#endif

         /* Allocate a connection */
         for (connection_num = 0; connection_num < netplay->connections_size; connection_num++)
            if (!netplay->connections[connection_num].active) break;
         if (connection_num == netplay->connections_size)
         {
            if (connection_num == 0)
            {
               netplay->connections = (struct netplay_connection*)malloc(sizeof(struct netplay_connection));
               if (netplay->connections == NULL)
               {
                  socket_close(new_fd);
                  goto process;
               }
               netplay->connections_size = 1;

            }
            else
            {
               size_t new_connections_size = netplay->connections_size * 2;
               struct netplay_connection *new_connections = (struct netplay_connection*)
                  realloc(netplay->connections,
                     new_connections_size*sizeof(struct netplay_connection));
               if (new_connections == NULL)
               {
                  socket_close(new_fd);
                  goto process;
               }

               memset(new_connections + netplay->connections_size, 0,
                  netplay->connections_size * sizeof(struct netplay_connection));
               netplay->connections = new_connections;
               netplay->connections_size = new_connections_size;

            }
         }
         connection = &netplay->connections[connection_num];

         /* Set it up */
         memset(connection, 0, sizeof(*connection));
         connection->active = true;
         connection->fd = new_fd;
         connection->mode = NETPLAY_CONNECTION_INIT;

         if (!netplay_init_socket_buffer(&connection->send_packet_buffer,
               netplay->packet_buffer_size) ||
             !netplay_init_socket_buffer(&connection->recv_packet_buffer,
               netplay->packet_buffer_size))
         {
            if (connection->send_packet_buffer.data)
               netplay_deinit_socket_buffer(&connection->send_packet_buffer);
            connection->active = false;
            socket_close(new_fd);
            goto process;
         }

         netplay_handshake_init_send(netplay, connection);

      }
   }

process:
   netplay->can_poll = true;
   input_poll_net();

   return (netplay->stall != NETPLAY_STALL_NO_CONNECTION);
}

/**
 * netplay_sync_post_frame
 * @netplay              : pointer to netplay object
 *
 * Post-frame for Netplay synchronization.
 * We check if we have new input and replay from recorded input.
 */
void netplay_sync_post_frame(netplay_t *netplay, bool stalled)
{
   uint32_t lo_frame_count, hi_frame_count;

   /* Unless we're stalling, we've just finished running a frame */
   if (!stalled)
   {
      netplay->self_ptr = NEXT_PTR(netplay->self_ptr);
      netplay->self_frame_count++;
   }

   /* Only relevant if we're connected */
   if ((netplay->is_server && !netplay->connected_players) ||
       (netplay->self_mode < NETPLAY_CONNECTION_CONNECTED))
   {
      netplay->other_frame_count = netplay->self_frame_count;
      netplay->other_ptr = netplay->self_ptr;
      /* FIXME: Duplication */
      if (netplay->catch_up)
      {
         netplay->catch_up = false;
         input_driver_unset_nonblock_state();
         driver_ctl(RARCH_DRIVER_CTL_SET_NONBLOCK_STATE, NULL);
      }
      return;
   }

#ifndef DEBUG_NONDETERMINISTIC_CORES
   if (!netplay->force_rewind)
   {
      /* Skip ahead if we predicted correctly.
       * Skip until our simulation failed. */
      while (netplay->other_frame_count < netplay->unread_frame_count &&
             netplay->other_frame_count < netplay->self_frame_count)
      {
         struct delta_frame *ptr = &netplay->buffer[netplay->other_ptr];
         size_t i;

         for (i = 0; i < MAX_USERS; i++)
         {
            if (memcmp(ptr->simulated_input_state[i], ptr->real_input_state[i],
                     sizeof(ptr->real_input_state[i])) != 0
                  && !ptr->used_real[i])
               break;
         }
         if (i != MAX_USERS) break;
         netplay_handle_frame_hash(netplay, ptr);
         netplay->other_ptr = NEXT_PTR(netplay->other_ptr);
         netplay->other_frame_count++;
      }
   }
#endif

   /* Now replay the real input if we've gotten ahead of it */
   if (netplay->force_rewind ||
       (netplay->other_frame_count < netplay->unread_frame_count &&
        netplay->other_frame_count < netplay->self_frame_count))
   {
      retro_ctx_serialize_info_t serial_info;

      /* Replay frames. */
      netplay->is_replay = true;
      netplay->replay_ptr = netplay->other_ptr;
      netplay->replay_frame_count = netplay->other_frame_count;

      if (netplay->quirks & NETPLAY_QUIRK_INITIALIZATION)
         /* Make sure we're initialized before we start loading things */
         netplay_wait_and_init_serialization(netplay);

      serial_info.data       = NULL;
      serial_info.data_const = netplay->buffer[netplay->replay_ptr].state;
      serial_info.size       = netplay->state_size;

      if (!core_unserialize(&serial_info))
      {
         RARCH_ERR("Netplay savestate loading failed: Prepare for desync!\n");
      }

      while (netplay->replay_frame_count < netplay->self_frame_count)
      {
         retro_time_t start, tm;

         struct delta_frame *ptr = &netplay->buffer[netplay->replay_ptr];
         serial_info.data       = ptr->state;
         serial_info.size       = netplay->state_size;
         serial_info.data_const = NULL;

         start = cpu_features_get_time_usec();

         /* Remember the current state */
         memset(serial_info.data, 0, serial_info.size);
         core_serialize(&serial_info);
         if (netplay->replay_frame_count < netplay->unread_frame_count)
            netplay_handle_frame_hash(netplay, ptr);

         /* Re-simulate this frame's input */
         netplay_simulate_input(netplay, netplay->replay_ptr, true);

         autosave_lock();
         core_run();
         autosave_unlock();
         netplay->replay_ptr = NEXT_PTR(netplay->replay_ptr);
         netplay->replay_frame_count++;

#ifdef DEBUG_NONDETERMINISTIC_CORES
         if (ptr->have_remote && netplay_delta_frame_ready(netplay, &netplay->buffer[netplay->replay_ptr], netplay->replay_frame_count))
         {
            RARCH_LOG("PRE  %u: %X\n", netplay->replay_frame_count-1, netplay_delta_frame_crc(netplay, ptr));
            if (netplay->is_server)
               RARCH_LOG("INP  %X %X\n", ptr->real_input_state[0], ptr->self_state[0]);
            else
               RARCH_LOG("INP  %X %X\n", ptr->self_state[0], ptr->real_input_state[0]);
            ptr = &netplay->buffer[netplay->replay_ptr];
            serial_info.data = ptr->state;
            memset(serial_info.data, 0, serial_info.size);
            core_serialize(&serial_info);
            RARCH_LOG("POST %u: %X\n", netplay->replay_frame_count-1, netplay_delta_frame_crc(netplay, ptr));
         }
#endif

         /* Get our time window */
         tm = cpu_features_get_time_usec() - start;
         netplay->frame_run_time_sum -= netplay->frame_run_time[netplay->frame_run_time_ptr];
         netplay->frame_run_time[netplay->frame_run_time_ptr] = tm;
         netplay->frame_run_time_sum += tm;
         netplay->frame_run_time_ptr++;
         if (netplay->frame_run_time_ptr >= NETPLAY_FRAME_RUN_TIME_WINDOW)
            netplay->frame_run_time_ptr = 0;
      }

      /* Average our time */
      netplay->frame_run_time_avg = netplay->frame_run_time_sum / NETPLAY_FRAME_RUN_TIME_WINDOW;

      if (netplay->unread_frame_count < netplay->self_frame_count)
      {
         netplay->other_ptr = netplay->unread_ptr;
         netplay->other_frame_count = netplay->unread_frame_count;
      }
      else
      {
         netplay->other_ptr = netplay->self_ptr;
         netplay->other_frame_count = netplay->self_frame_count;
      }
      netplay->is_replay = false;
      netplay->force_rewind = false;
   }

   if (netplay->is_server)
   {
      uint32_t player;

      lo_frame_count = hi_frame_count = netplay->unread_frame_count;

      /* Look for players that are ahead of us */
      for (player = 0; player < MAX_USERS; player++)
      {
         if (!(netplay->connected_players & (1<<player))) continue;
         if (netplay->read_frame_count[player] > hi_frame_count)
            hi_frame_count = netplay->read_frame_count[player];
      }
   }
   else
   {
      lo_frame_count = hi_frame_count = netplay->server_frame_count;
   }

   /* If we're behind, try to catch up */
   if (netplay->catch_up)
   {
      /* Are we caught up? */
      if (netplay->self_frame_count >= lo_frame_count)
      {
         netplay->catch_up = false;
         input_driver_unset_nonblock_state();
         driver_ctl(RARCH_DRIVER_CTL_SET_NONBLOCK_STATE, NULL);
      }

   }
   else if (!stalled)
   {
      if (netplay->self_frame_count + 2 < lo_frame_count)
      {
         /* Are we falling behind? */
         netplay->catch_up = true;
         input_driver_set_nonblock_state();
         driver_ctl(RARCH_DRIVER_CTL_SET_NONBLOCK_STATE, NULL);

      }
      else if (netplay->self_frame_count + 2 < hi_frame_count)
      {
         size_t i;

         /* We're falling behind some clients but not others, so request that
          * clients ahead of us stall */
         for (i = 0; i < netplay->connections_size; i++)
         {
            struct netplay_connection *connection = &netplay->connections[i];
            int player;
            if (!connection->active ||
                connection->mode != NETPLAY_CONNECTION_PLAYING)
               continue;
            player = connection->player;

            /* Are they ahead? */
            if (netplay->self_frame_count + 2 < netplay->read_frame_count[player])
            {
               /* Tell them to stall */
               if (connection->stall_frame + NETPLAY_MAX_REQ_STALL_FREQUENCY <
                     netplay->self_frame_count)
               {
                  connection->stall_frame = netplay->self_frame_count;
                  netplay_cmd_stall(netplay, connection,
                     netplay->read_frame_count[player] -
                     netplay->self_frame_count + 1);
               }
            }
         }
      }
   }
}
