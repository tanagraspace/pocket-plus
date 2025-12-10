/*
 * Copyright (c) 2025 Tanagra Space
 * SPDX-License-Identifier: MIT
 */

package space.tanagra.pocketplus;

/**
 * Exception thrown by POCKET+ compression/decompression operations.
 *
 * <p>This exception indicates an error during POCKET+ processing, such as invalid parameters,
 * buffer overflow/underflow, or corrupted data.
 */
public class PocketException extends Exception {

  private static final long serialVersionUID = 1L;

  /**
   * Constructs a new PocketException with the specified message.
   *
   * @param message the detail message
   */
  public PocketException(String message) {
    super(message);
  }

  /**
   * Constructs a new PocketException with the specified message and cause.
   *
   * @param message the detail message
   * @param cause the cause of this exception
   */
  public PocketException(String message, Throwable cause) {
    super(message, cause);
  }
}
