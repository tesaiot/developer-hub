import 'package:flutter_test/flutter_test.dart';
import 'package:tesaiot_sso_example/models/user_model.dart';

/// Tests for the UserModel class.
///
/// These tests verify user model functionality including:
/// - Creation from JWT claims
/// - Role checking
/// - Display name generation
/// - Initials generation
/// - JSON serialization
void main() {
  group('UserModel', () {
    group('creation', () {
      test('creates with required fields', () {
        final user = UserModel(
          id: 'user-123',
          username: 'testuser',
        );

        expect(user.id, equals('user-123'));
        expect(user.username, equals('testuser'));
        expect(user.roles, isEmpty);
        expect(user.emailVerified, isFalse);
      });

      test('creates with all fields', () {
        final user = UserModel(
          id: 'user-123',
          username: 'testuser',
          email: 'test@example.com',
          name: 'Test User',
          givenName: 'Test',
          familyName: 'User',
          emailVerified: true,
          organizationId: 'org-456',
          organizationName: 'Test Organization',
          roles: ['user', 'admin'],
        );

        expect(user.id, equals('user-123'));
        expect(user.username, equals('testuser'));
        expect(user.email, equals('test@example.com'));
        expect(user.name, equals('Test User'));
        expect(user.givenName, equals('Test'));
        expect(user.familyName, equals('User'));
        expect(user.emailVerified, isTrue);
        expect(user.organizationId, equals('org-456'));
        expect(user.organizationName, equals('Test Organization'));
        expect(user.roles, equals(['user', 'admin']));
      });
    });

    group('fromClaims', () {
      test('creates from minimal claims', () {
        final claims = {
          'sub': 'user-123',
          'preferred_username': 'testuser',
        };

        final user = UserModel.fromClaims(claims);

        expect(user.id, equals('user-123'));
        expect(user.username, equals('testuser'));
        expect(user.roles, isEmpty);
      });

      test('creates from full claims', () {
        final claims = {
          'sub': 'user-123',
          'preferred_username': 'testuser',
          'email': 'test@example.com',
          'name': 'Test User',
          'given_name': 'Test',
          'family_name': 'User',
          'email_verified': true,
          'organization_id': 'org-456',
          'organization_name': 'Test Org',
          'realm_access': {
            'roles': ['user', 'admin'],
          },
          'resource_access': {
            'test-client': {
              'roles': ['device-manager'],
            },
          },
        };

        final user = UserModel.fromClaims(claims, clientId: 'test-client');

        expect(user.id, equals('user-123'));
        expect(user.username, equals('testuser'));
        expect(user.email, equals('test@example.com'));
        expect(user.name, equals('Test User'));
        expect(user.givenName, equals('Test'));
        expect(user.familyName, equals('User'));
        expect(user.emailVerified, isTrue);
        expect(user.organizationId, equals('org-456'));
        expect(user.organizationName, equals('Test Org'));
        expect(user.roles, contains('user'));
        expect(user.roles, contains('admin'));
        expect(user.roles, contains('device-manager'));
      });

      test('extracts realm roles correctly', () {
        final claims = {
          'sub': 'user-123',
          'preferred_username': 'testuser',
          'realm_access': {
            'roles': ['user', 'admin', 'viewer'],
          },
        };

        final user = UserModel.fromClaims(claims);

        expect(user.roles.length, equals(3));
        expect(user.roles, contains('user'));
        expect(user.roles, contains('admin'));
        expect(user.roles, contains('viewer'));
      });

      test('extracts client roles correctly', () {
        final claims = {
          'sub': 'user-123',
          'preferred_username': 'testuser',
          'resource_access': {
            'my-client': {
              'roles': ['role1', 'role2'],
            },
          },
        };

        final user = UserModel.fromClaims(claims, clientId: 'my-client');

        expect(user.roles, contains('role1'));
        expect(user.roles, contains('role2'));
      });

      test('deduplicates roles from realm and client', () {
        final claims = {
          'sub': 'user-123',
          'preferred_username': 'testuser',
          'realm_access': {
            'roles': ['admin', 'user'],
          },
          'resource_access': {
            'test-client': {
              'roles': ['admin', 'manager'], // 'admin' is duplicate
            },
          },
        };

        final user = UserModel.fromClaims(claims, clientId: 'test-client');

        expect(user.roles.length, equals(3));
        expect(user.roles, contains('admin'));
        expect(user.roles, contains('user'));
        expect(user.roles, contains('manager'));
      });

      test('handles missing realm_access gracefully', () {
        final claims = {
          'sub': 'user-123',
          'preferred_username': 'testuser',
        };

        final user = UserModel.fromClaims(claims);

        expect(user.roles, isEmpty);
      });

      test('handles missing resource_access gracefully', () {
        final claims = {
          'sub': 'user-123',
          'preferred_username': 'testuser',
          'realm_access': {
            'roles': ['user'],
          },
        };

        final user = UserModel.fromClaims(claims, clientId: 'nonexistent-client');

        expect(user.roles, equals(['user']));
      });

      test('handles empty roles arrays', () {
        final claims = {
          'sub': 'user-123',
          'preferred_username': 'testuser',
          'realm_access': {
            'roles': <String>[],
          },
        };

        final user = UserModel.fromClaims(claims);

        expect(user.roles, isEmpty);
      });

      test('uses empty string for missing preferred_username', () {
        final claims = {
          'sub': 'user-123',
        };

        final user = UserModel.fromClaims(claims);

        expect(user.username, equals(''));
      });

      test('handles email_verified as false by default', () {
        final claims = {
          'sub': 'user-123',
          'preferred_username': 'testuser',
        };

        final user = UserModel.fromClaims(claims);

        expect(user.emailVerified, isFalse);
      });
    });

    group('displayName', () {
      test('returns name when available', () {
        final user = UserModel(
          id: 'user-123',
          username: 'testuser',
          name: 'Test User',
        );

        expect(user.displayName, equals('Test User'));
      });

      test('returns username when name is null', () {
        final user = UserModel(
          id: 'user-123',
          username: 'testuser',
        );

        expect(user.displayName, equals('testuser'));
      });
    });

    group('initials', () {
      test('returns two initials from full name', () {
        final user = UserModel(
          id: 'user-123',
          username: 'testuser',
          name: 'John Doe',
        );

        expect(user.initials, equals('JD'));
      });

      test('returns single initial from single name', () {
        final user = UserModel(
          id: 'user-123',
          username: 'testuser',
          name: 'Madonna',
        );

        expect(user.initials, equals('M'));
      });

      test('returns username initial when name is null', () {
        final user = UserModel(
          id: 'user-123',
          username: 'testuser',
        );

        expect(user.initials, equals('T'));
      });

      test('returns uppercase initials', () {
        final user = UserModel(
          id: 'user-123',
          username: 'testuser',
          name: 'john doe',
        );

        expect(user.initials, equals('JD'));
      });

      test('handles multiple word names', () {
        final user = UserModel(
          id: 'user-123',
          username: 'testuser',
          name: 'John Jacob Jingleheimer Schmidt',
        );

        expect(user.initials, equals('JJ'));
      });
    });

    group('hasRole', () {
      test('returns true when user has role', () {
        final user = UserModel(
          id: 'user-123',
          username: 'testuser',
          roles: ['user', 'admin'],
        );

        expect(user.hasRole('admin'), isTrue);
      });

      test('returns false when user does not have role', () {
        final user = UserModel(
          id: 'user-123',
          username: 'testuser',
          roles: ['user'],
        );

        expect(user.hasRole('admin'), isFalse);
      });

      test('returns false when roles is empty', () {
        final user = UserModel(
          id: 'user-123',
          username: 'testuser',
        );

        expect(user.hasRole('user'), isFalse);
      });

      test('is case sensitive', () {
        final user = UserModel(
          id: 'user-123',
          username: 'testuser',
          roles: ['Admin'],
        );

        expect(user.hasRole('admin'), isFalse);
        expect(user.hasRole('Admin'), isTrue);
      });
    });

    group('hasAnyRole', () {
      test('returns true when user has one of the roles', () {
        final user = UserModel(
          id: 'user-123',
          username: 'testuser',
          roles: ['user', 'viewer'],
        );

        expect(user.hasAnyRole(['admin', 'user']), isTrue);
      });

      test('returns false when user has none of the roles', () {
        final user = UserModel(
          id: 'user-123',
          username: 'testuser',
          roles: ['viewer'],
        );

        expect(user.hasAnyRole(['admin', 'user']), isFalse);
      });

      test('returns false when roles list is empty', () {
        final user = UserModel(
          id: 'user-123',
          username: 'testuser',
          roles: ['user'],
        );

        expect(user.hasAnyRole([]), isFalse);
      });

      test('returns true when user has multiple matching roles', () {
        final user = UserModel(
          id: 'user-123',
          username: 'testuser',
          roles: ['user', 'admin', 'viewer'],
        );

        expect(user.hasAnyRole(['admin', 'user']), isTrue);
      });
    });

    group('JSON serialization', () {
      test('converts to JSON correctly', () {
        final user = UserModel(
          id: 'user-123',
          username: 'testuser',
          email: 'test@example.com',
          name: 'Test User',
          givenName: 'Test',
          familyName: 'User',
          emailVerified: true,
          organizationId: 'org-456',
          organizationName: 'Test Org',
          roles: ['user', 'admin'],
        );

        final json = user.toJson();

        expect(json['id'], equals('user-123'));
        expect(json['username'], equals('testuser'));
        expect(json['email'], equals('test@example.com'));
        expect(json['name'], equals('Test User'));
        expect(json['givenName'], equals('Test'));
        expect(json['familyName'], equals('User'));
        expect(json['emailVerified'], isTrue);
        expect(json['organizationId'], equals('org-456'));
        expect(json['organizationName'], equals('Test Org'));
        expect(json['roles'], equals(['user', 'admin']));
      });

      test('converts from JSON correctly', () {
        final json = {
          'id': 'user-123',
          'username': 'testuser',
          'email': 'test@example.com',
          'name': 'Test User',
          'givenName': 'Test',
          'familyName': 'User',
          'emailVerified': true,
          'organizationId': 'org-456',
          'organizationName': 'Test Org',
          'roles': ['user', 'admin'],
        };

        final user = UserModel.fromJson(json);

        expect(user.id, equals('user-123'));
        expect(user.username, equals('testuser'));
        expect(user.email, equals('test@example.com'));
        expect(user.name, equals('Test User'));
        expect(user.givenName, equals('Test'));
        expect(user.familyName, equals('User'));
        expect(user.emailVerified, isTrue);
        expect(user.organizationId, equals('org-456'));
        expect(user.organizationName, equals('Test Org'));
        expect(user.roles, equals(['user', 'admin']));
      });

      test('handles null optional fields in JSON', () {
        final json = {
          'id': 'user-123',
          'username': 'testuser',
          'email': null,
          'name': null,
          'givenName': null,
          'familyName': null,
          'emailVerified': false,
          'organizationId': null,
          'organizationName': null,
          'roles': <String>[],
        };

        final user = UserModel.fromJson(json);

        expect(user.email, isNull);
        expect(user.name, isNull);
        expect(user.givenName, isNull);
        expect(user.familyName, isNull);
        expect(user.organizationId, isNull);
        expect(user.organizationName, isNull);
        expect(user.roles, isEmpty);
      });

      test('uses default values for missing fields', () {
        final json = {
          'id': 'user-123',
          'username': 'testuser',
        };

        final user = UserModel.fromJson(json);

        expect(user.emailVerified, isFalse);
        expect(user.roles, isEmpty);
      });
    });

    group('Equatable', () {
      test('equal users have same props', () {
        final user1 = UserModel(
          id: 'user-123',
          username: 'testuser',
          email: 'test@example.com',
          roles: ['user'],
        );
        final user2 = UserModel(
          id: 'user-123',
          username: 'testuser',
          email: 'test@example.com',
          roles: ['user'],
        );

        expect(user1, equals(user2));
      });

      test('different users have different props', () {
        final user1 = UserModel(
          id: 'user-123',
          username: 'testuser1',
        );
        final user2 = UserModel(
          id: 'user-123',
          username: 'testuser2',
        );

        expect(user1, isNot(equals(user2)));
      });

      test('users with different roles are not equal', () {
        final user1 = UserModel(
          id: 'user-123',
          username: 'testuser',
          roles: ['user'],
        );
        final user2 = UserModel(
          id: 'user-123',
          username: 'testuser',
          roles: ['admin'],
        );

        expect(user1, isNot(equals(user2)));
      });
    });
  });
}
